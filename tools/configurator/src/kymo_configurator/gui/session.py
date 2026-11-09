# SPDX-License-Identifier: MIT
"""Shared editing state for both GUI modes.

Both the guided and the professional mode edit the same ProjectSession, so
switching modes never loses data. All writes go through catalog fields.
"""
from __future__ import annotations

import copy
import difflib
from pathlib import Path
from typing import Any

from PySide6.QtCore import QObject, Signal

from .. import catalog
from ..generation import _linked, _render, generate, selected_generation
from ..i18n import tr
from ..migration import migrate, needs_migration
from ..model import default_project, load_project, save_project
from ..resources import estimate_ram
from ..validation import Diagnostic, PROFILE_NAME, validate


class ProjectSession(QObject):
    """One open project document plus the selected profile."""

    changed = Signal()
    """A value changed; editors refresh enabled state, summaries and diagnostics."""
    replaced = Signal()
    """The document or active profile was replaced; editors reload everything."""

    def __init__(self, doc: dict[str, Any] | None = None, path: Path | None = None):
        super().__init__()
        self.path: Path | None = path
        self.notes: list[str] = []
        self.dirty = False
        self._diagnostics: list[Diagnostic] | None = None
        self.doc = self._prepare(doc if doc is not None else default_project())

    # --- document lifecycle -------------------------------------------------
    def _prepare(self, doc: dict[str, Any]) -> dict[str, Any]:
        migrated = needs_migration(doc)
        if migrated:
            doc = migrate(doc)
        invalid_shape = {
            "schema.type", "schema.version", "profiles.empty", "profile.type", "features.type",
            "trace.type", "trace.section_type", "build.type", "resources.type", "bindings.type",
            "port.type", "watch.type", "channels.type", "channel.type", "registry.type", "registry.row",
        }
        problems = [d for d in validate(doc) if d.code in invalid_shape]
        if problems:
            raise ValueError("\n".join(f"{d.path}: {d.localized()[0]}" for d in problems))
        self.notes = []
        self.dirty = migrated
        if migrated:
            self.notes.append(tr("This project was written for KymoCore 6.x and has been upgraded "
                                 "to the KymoCore 7.0 format. Save to keep the upgrade."))
        return doc

    def new(self) -> None:
        self.doc = default_project()
        self.path = None
        self.notes = []
        self.dirty = False
        self._invalidate(replaced=True)

    def load(self, path: str | Path) -> None:
        doc = load_project(path)
        self.doc = self._prepare(doc)
        self.path = Path(path)
        self._invalidate(replaced=True)

    def save(self, path: str | Path | None = None) -> Path:
        target = Path(path) if path else self.path
        if target is None:
            raise ValueError("No file name selected")
        save_project(target, self.doc)
        self.path = target
        self.dirty = False
        self.notes = []
        self.changed.emit()
        return target

    # --- profile ------------------------------------------------------------
    @property
    def profiles(self) -> list[str]:
        profiles = self.doc.get("profiles", {})
        return list(profiles) if isinstance(profiles, dict) else []

    @property
    def profile(self) -> str:
        name = self.doc.get("active_profile")
        if name not in self.profiles and self.profiles:
            name = self.profiles[0]
        return name or ""

    def set_profile(self, name: str) -> None:
        if name in self.profiles and name != self.doc.get("active_profile"):
            self.doc["active_profile"] = name
            self.dirty = True
            self._invalidate(replaced=True)

    def duplicate_profile(self, name: str | None = None) -> str:
        """Copy the active profile under ``name`` (or a free ``<profile>_copy`` name) and select it."""
        source = self.profile
        if name is None:
            base = source[:59] + "_copy"
            name = base
            index = 2
            while name in self.profiles:
                suffix = str(index)
                name = base[:64 - len(suffix)] + suffix
                index += 1
        elif not PROFILE_NAME.fullmatch(name) or name in self.profiles:
            raise ValueError("Invalid or duplicate profile name")
        self.doc["profiles"][name] = copy.deepcopy(self.doc["profiles"][source])
        self.doc["active_profile"] = name
        self.dirty = True
        self._invalidate(replaced=True)
        return name

    # --- values -------------------------------------------------------------
    def view(self) -> catalog.View:
        return catalog.View(self.doc, self.profile)

    def value(self, field: catalog.Field) -> Any:
        return field.value(self.view())

    def set_value(self, field: catalog.Field, value: Any) -> None:
        field.store(self.doc, self.profile, value)
        self.touch()

    def touch(self) -> None:
        """Record a direct document edit (tables and custom widgets)."""
        self.dirty = True
        self._invalidate()

    def _invalidate(self, replaced: bool = False) -> None:
        self._diagnostics = None
        if replaced:
            self.replaced.emit()
        self.changed.emit()

    # --- validation, preview, generation --------------------------------------
    def diagnostics(self) -> list[Diagnostic]:
        if self._diagnostics is None:
            self._diagnostics = validate(self.doc)
        return self._diagnostics

    def errors(self) -> list[Diagnostic]:
        return [d for d in self.diagnostics() if d.severity == "error"]

    def diagnostics_for(self, section: catalog.Section) -> list[Diagnostic]:
        return [d for d in self.diagnostics()
                if catalog.section_for_path(d.path, self.profile) is section]

    def concrete_path(self, field: catalog.Field) -> str:
        return ".".join(catalog._resolve(field.path, self.profile))

    def output_path(self) -> Path:
        configured = Path(self.doc.get("build", {}).get("output_dir", "generated/kymo"))
        base = (self.path.parent if self.path else Path.cwd()) / configured
        return base / self.profile

    def ram_parts(self) -> dict[str, int] | None:
        """A malformed configuration has no reliable estimate."""
        if self.errors():
            return None
        return estimate_ram(self.doc, self.profile)

    def effective_macros(self) -> list[tuple[str, int]]:
        resolved = self.view().resolved()
        macros = [("KYMO_ENABLED", int(resolved["enabled"]))]
        macros += [(f"KYMO_ENABLE_{k.upper()}", int(v)) for k, v in resolved["effective_features"].items()]
        trace = resolved["effective_trace"]
        macros.append(("KYMO_ENABLE_TRACE", int(trace["enabled"])))
        macros += [(f"KYMO_ENABLE_TRACE_{k.upper()}", int(trace.get(k, False)))
                   for k in ("isr", "scheduler", "flow")]
        macros.append(("KYMO_ENABLE_TRACE_STACK_POINTER", int(
            trace["enabled"] and bool(self.view().get((catalog.PROFILE, "trace", "stack_pointer"), False)))))
        channels = self.doc.get("channels", [])
        macros.append(("KYMO_MAX_CHANNELS", max(1, len(channels) if isinstance(channels, list) else 1)))
        return macros

    def preview(self) -> dict[str, tuple[str, list[str]]]:
        """Generated file name -> (change state, unified diff or content lines)."""
        rendered = _render(self.doc, self.profile)
        previous = None
        try:
            previous, _ = selected_generation(self.output_path())
        except (OSError, ValueError):
            pass
        result = {}
        for name, data in sorted(rendered.items()):
            new = data.decode("utf-8").splitlines()
            old_file = previous / name if previous else None
            old = None
            if old_file is not None and not _linked(old_file):
                try:
                    old = old_file.read_text(encoding="utf-8").splitlines()
                except (OSError, UnicodeError):
                    pass
            if old is None:
                result[name] = ("new", new)
            elif old == new:
                result[name] = ("unchanged", new)
            else:
                result[name] = ("changed", list(difflib.unified_diff(
                    old, new, fromfile="previous", tofile="proposed", lineterm="")))
        return result

    def generate(self) -> Path:
        if errors := self.errors():
            raise ValueError("\n".join(f"{d.path}: {d.localized()[0]}" for d in errors))
        snapshot = generate(self.doc, self.profile, self.output_path())
        self.changed.emit()
        return snapshot
