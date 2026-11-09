# SPDX-License-Identifier: MIT
"""Create an explicitly requested, never-overwriting callback scaffold."""
from __future__ import annotations
from pathlib import Path
from typing import Any

from .resolution import resolve_profile
from .trace_config import trace_callbacks
from .validation import validate


def render_port_template(project: dict[str, Any]) -> dict[str, str]:
    errors = [f"{d.path}: {d.message}" for d in validate(project) if d.severity == "error"]
    if errors:
        raise ValueError("\n".join(errors))
    resolved = resolve_profile(project, project["active_profile"])
    bindings = resolved["bindings"]
    trace = resolved["requested_trace"]
    tracing = resolved["effective_trace"]["enabled"]
    shared = tracing and trace.get("transport", {}).get("mode") == "shared_v2"
    callbacks = trace_callbacks(trace)
    runtime_stubs = [
        (bindings.get("clock"), "uint32_t", "void *user", "return a monotonic millisecond counter"),
        (bindings.get("sample"), "uint8_t", "void *user, uint8_t id, KymoSample *sample",
         "copy a measurement and return nonzero only when available"),
        (None if shared else bindings.get("write"), "uint8_t",
         "void *user, const uint8_t *bytes, uint8_t length", "accept and report a prefix of the provided bytes"),
        (None if shared else bindings.get("busy"), "uint8_t", "void *user",
         "report whether a borrowed TX buffer is still in use"),
        (bindings.get("service"), "void", "void *user", "perform bounded transport service, if required"),
    ]
    trace_stubs = [
        (callbacks["clock"], "uint64_t", "void *user", "return the free-running trace tick counter"),
        (callbacks["session_id"], "uint64_t", "void", "return a new random session ID per boot"),
        (callbacks["trace_write"], "size_t", "void *user, const uint8_t *bytes, size_t length",
         "write a prefix of the mux frame and return its length"),
        (callbacks.get("transport_busy"), "uint8_t", "void *user", "report nonzero while the driver owns the frame"),
        (callbacks.get("core_id"), "uint8_t", "void *user", "return the ID of the executing core"),
        (callbacks.get("current_context"), "uint32_t", "void *user", "return the current context ID"),
        (callbacks.get("critical_enter"), "void", "void *user", "enter a recorder critical section"),
        (callbacks.get("critical_exit"), "void", "void *user", "leave the recorder critical section"),
    ]

    header = ["#pragma once", "#include <KymoCore.h>", "#ifdef __cplusplus", 'extern "C" {', "#endif"]
    source = ["/* User-owned starter only. Replace each TODO with target-specific behavior. */",
              '#include "kymo_port.h"']
    # A shared callback must remain available when any consuming feature is on,
    # including when compiler definitions disable another feature in this profile.
    functions = {}
    for feature, enabled, stubs in (
        ("RUNTIME", resolved["effective_features"]["runtime"], runtime_stubs),
        ("TRACE", tracing, trace_stubs),
    ):
        if not enabled:
            continue
        guard = f"KYMO_EFFECTIVE_{feature}"
        source.extend((f"#if {guard}",
                       f"#error Implement the Kymo port callbacks before enabling {feature.lower()}", "#endif"))
        for name, result, params, todo in stubs:
            if not name:
                continue
            if name not in functions:
                functions[name] = (result, params, todo, [])
            guards = functions[name][3]
            if guard not in guards:
                guards.append(guard)
    for name, (result, params, todo, guards) in functions.items():
        guard = " || ".join(guards)
        declaration = f"{result} {name}({params})"
        header.extend((f"#if {guard}", declaration + ";", "#endif"))
        source.extend((f"#if {guard}", "/" + "*" * 79, f" * {name}",
                       " " + "*" * 78 + "/", f'extern "C" {declaration} {{'))
        if params != "void":
            for param in params.split(", "):
                source.append(f"    (void){param.split()[-1].lstrip('*')};")
        source.append(f"    /* TODO: {todo}. */")
        if result != "void":
            source.append("    return 0u;")
        source.extend(("}", "#endif"))
    header.extend(("#ifdef __cplusplus", "}", "#endif"))
    return {"kymo_port.h": "\n".join(header) + "\n", "kymo_port.cpp": "\n".join(source) + "\n"}


def create_port_template(project: dict[str, Any], destination: str | Path) -> Path:
    target = Path(destination).absolute()
    files = render_port_template(project)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.mkdir()
    written = []
    try:
        for name, content in files.items():
            path = target / name
            with path.open("x", encoding="utf-8", newline="\n") as stream:
                written.append(path)
                stream.write(content)
    except OSError:
        for path in written:
            path.unlink(missing_ok=True)
        target.rmdir()
        raise
    return target
