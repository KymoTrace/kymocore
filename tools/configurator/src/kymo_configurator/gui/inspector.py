# SPDX-License-Identifier: MIT
"""Right-hand panel of the professional mode: RAM estimate, problems, generated files."""
from __future__ import annotations

from PySide6.QtCore import Qt, QTimer, Signal
from PySide6.QtWidgets import (QButtonGroup, QFrame, QGridLayout, QHBoxLayout, QLabel, QPlainTextEdit,
                               QPushButton, QScrollArea, QStackedWidget, QVBoxLayout, QWidget)

from .. import catalog
from ..i18n import tr
from . import style
from .highlight import CodeHighlighter
from .parts import StackedBar, chip, rule, set_chip
from .session import ProjectSession

def ram_parts() -> dict[str, tuple[str, str]]:
    """Translated display names and bar colours of the parts estimate_ram() reports."""
    return {
        "trace": (tr("Trace buffer"), style.PRIMARY),
        "mux": (tr("Multiplexer"), style.PRIMARY_BORDER),
        "watch": (tr("Watch variables"), style.SUCCESS),
        "runtime": (tr("Runtime and channels"), style.WARNING),
        "control": (tr("Return channel"), style.TEXT_SECONDARY),
        "parameters": (tr("Adjust values live"), style.TEXT_SECONDARY),
        "memory": (tr("Watch memory (stacks and heap)"), style.TEXT_SECONDARY),
    }
MAX_PROBLEM_CARDS = 6


def _bytes(value: int) -> str:
    return f"{value:,}".replace(",", " ") + " B"


def _label(text: str = "", name: str = "") -> QLabel:
    label = QLabel(text)
    label.setTextFormat(Qt.TextFormat.PlainText)
    if name:
        label.setObjectName(name)
    return label


class InspectorPanel(QWidget):
    """Tabs 'Diagnostics' and 'Preview'; refreshes shortly after the last edit."""

    navigate = Signal(str)

    def __init__(self, session: ProjectSession):
        super().__init__()
        self.session = session
        self.setObjectName("inspector")
        self.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        self.setFixedWidth(340)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(12)
        tabs = QHBoxLayout()
        tabs.setSpacing(4)
        self.tab_group = QButtonGroup(self)
        self.pages = QStackedWidget()
        for index, text in enumerate((tr("Diagnostics"), tr("Preview"))):
            button = QPushButton(text)
            button.setObjectName("tabButton")
            button.setCheckable(True)
            button.setChecked(index == 0)
            self.tab_group.addButton(button, index)
            tabs.addWidget(button)
        self.tab_group.idClicked.connect(self._show_tab)
        layout.addLayout(tabs)
        layout.addWidget(self.pages, 1)
        self.pages.addWidget(self._build_diagnostics())
        self.preview = QPlainTextEdit()
        self.preview.setObjectName("preview")
        self.preview.setReadOnly(True)
        self.preview.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)
        self._highlighter = CodeHighlighter(self.preview.document())
        self.pages.addWidget(self.preview)
        self._timer = QTimer(self)
        self._timer.setSingleShot(True)
        self._timer.setInterval(250)
        self._timer.timeout.connect(self.refresh)
        session.changed.connect(self._timer.start)
        self.refresh()

    def _build_diagnostics(self) -> QWidget:
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        body = QWidget()
        body.setObjectName("inspector")
        layout = QVBoxLayout(body)
        layout.setContentsMargins(0, 0, 4, 0)
        layout.setSpacing(10)
        head = QHBoxLayout()
        head.addWidget(_label(tr("RAM estimate"), "inspectorHeading"))
        head.addStretch(1)
        self.ram_total = _label("", "inspectorValue")
        head.addWidget(self.ram_total)
        layout.addLayout(head)
        self.ram_bar = StackedBar()
        layout.addWidget(self.ram_bar)
        self.ram_rows = QGridLayout()
        self.ram_rows.setHorizontalSpacing(12)
        self.ram_rows.setVerticalSpacing(3)
        layout.addLayout(self.ram_rows)
        layout.addWidget(_label(tr("Estimate for Cortex-M4, without your own buffers and stacks."),
                                "inspectorNote"))
        layout.addWidget(rule())
        problems_head = QHBoxLayout()
        problems_head.addWidget(_label(tr("Problems"), "inspectorHeading"))
        self.error_chip = chip("", "danger")
        self.warning_chip = chip("", "warning")
        problems_head.addWidget(self.error_chip)
        problems_head.addWidget(self.warning_chip)
        problems_head.addStretch(1)
        layout.addLayout(problems_head)
        self.problem_box = QVBoxLayout()
        self.problem_box.setSpacing(8)
        layout.addLayout(self.problem_box)
        layout.addWidget(rule())
        layout.addWidget(_label(tr("Generated files"), "inspectorHeading"))
        self.file_rows = QGridLayout()
        self.file_rows.setHorizontalSpacing(12)
        self.file_rows.setVerticalSpacing(4)
        layout.addLayout(self.file_rows)
        layout.addStretch(1)
        scroll.setWidget(body)
        return scroll

    def _show_tab(self, index: int) -> None:
        self.pages.setCurrentIndex(index)
        self.refresh()

    @staticmethod
    def _clear(layout) -> None:
        while layout.count():
            item = layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

    def refresh(self) -> None:
        if not self.isVisibleTo(self.window()) and self.parentWidget() is not None:
            return
        self._refresh_ram()
        self._refresh_problems()
        self._refresh_files()

    def showEvent(self, event) -> None:  # noqa: N802 - Qt override
        super().showEvent(event)
        self.refresh()

    def _refresh_ram(self) -> None:
        parts = self.session.ram_parts()
        self._clear(self.ram_rows)
        if parts is None:
            self.ram_total.setText("—")
            self.ram_bar.set_parts([])
            return
        total = sum(parts.values())
        budget = (self.session.doc.get("resources") or {}).get("ram_budget_bytes")
        scale = budget if budget is not None else max(total, 1)
        self.ram_total.setText(f"{_bytes(total)} / {_bytes(budget)}" if budget is not None else _bytes(total))
        ordered = sorted(parts.items(), key=lambda item: -item[1])
        names = ram_parts()
        self.ram_bar.set_parts([(value / scale, names.get(key, (key, style.TEXT_HINT))[1])
                                for key, value in ordered])
        for row, (key, value) in enumerate(ordered):
            self.ram_rows.addWidget(_label(names.get(key, (key, ""))[0], "inspectorSmall"), row, 0)
            self.ram_rows.addWidget(_label(_bytes(value), "inspectorValue"), row, 1)
        if not parts:
            self.ram_rows.addWidget(_label(tr("KymoCore is switched off in this profile."), "inspectorSmall"), 0, 0)

    def _refresh_problems(self) -> None:
        diagnostics = self.session.diagnostics()
        errors = [d for d in diagnostics if d.severity == "error"]
        warnings = [d for d in diagnostics if d.severity != "error"]
        set_chip(self.error_chip, tr("{count} problem(s) to fix", count=len(errors)), "danger")
        set_chip(self.warning_chip, tr("{count} warning(s)", count=len(warnings)), "warning")
        self.error_chip.setVisible(bool(errors))
        self.warning_chip.setVisible(bool(warnings))
        self._clear(self.problem_box)
        for diagnostic in (errors + warnings)[:MAX_PROBLEM_CARDS]:
            owner = catalog.section_for_path(diagnostic.path, self.session.profile)
            message, suggestion = diagnostic.localized()
            card = QFrame()
            card.setObjectName("card_danger" if diagnostic.severity == "error" else "card_warning")
            inner = QVBoxLayout(card)
            inner.setContentsMargins(10, 8, 10, 8)
            inner.setSpacing(3)
            title = _label(f"{tr(owner.title)}: {message}" if owner else message, "cardTitle")
            title.setWordWrap(True)
            inner.addWidget(title)
            if suggestion:
                text = _label(suggestion, "cardText")
                text.setWordWrap(True)
                inner.addWidget(text)
            if owner:
                jump = QPushButton(tr("Go to the field"))
                jump.setObjectName("linkButton")
                jump.clicked.connect(lambda _=False, k=owner.key: self.navigate.emit(k))
                inner.addWidget(jump)
            self.problem_box.addWidget(card)
        hidden = len(diagnostics) - MAX_PROBLEM_CARDS
        if hidden > 0:
            more = QPushButton(tr("{count} more in 'Review and generate'", count=hidden))
            more.setObjectName("linkButton")
            more.clicked.connect(lambda: self.navigate.emit("review"))
            self.problem_box.addWidget(more)
        if not diagnostics:
            self.problem_box.addWidget(_label(tr("✓ No problems found. The configuration can be generated."),
                                              "inspectorSmall"))

    def _refresh_files(self) -> None:
        self._clear(self.file_rows)
        if self.session.errors():
            note = _label(tr("Shown once the open problems are fixed."), "inspectorSmall")
            self.file_rows.addWidget(note, 0, 0)
            self.preview.setPlainText(tr("Resolve the open problems first."))
            return
        preview = self.session.preview()
        names = {"new": tr("new"), "changed": tr("changed"), "unchanged": tr("unchanged")}
        lines = []
        for row, (name, (state, content)) in enumerate(preview.items()):
            self.file_rows.addWidget(_label(name, "fileName"), row, 0)
            self.file_rows.addWidget(_label(names.get(state, state), f"fileState_{state}"),
                                     row, 1)
            if state != "unchanged":
                lines.append(f"--- {name} ({names.get(state, state)}) ---")
                lines.extend(content[:80])
                lines.append("")
        if self.pages.currentIndex() == 1:
            self._highlighter.set_diff(any(state == "changed" for state, _ in preview.values()))
            self.preview.setPlainText("\n".join(lines) or tr("No changes against the last generated files."))
