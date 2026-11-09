# SPDX-License-Identifier: MIT
"""Trace configuration shared by validation, generation and the RAM estimate."""
from __future__ import annotations

import hashlib
from typing import Any

from .model import canonical

TRACE_ADMIN_MAX = 8192


def trace_callbacks(trace: dict[str, Any]) -> dict[str, Any]:
    """Resolve legacy callback locations and aliases into the current binding names."""
    bindings = trace.get("bindings", {})
    return {
        **bindings,
        "clock": bindings.get("clock") or trace.get("clock", {}).get("callback"),
        "session_id": bindings.get("session_id") or trace.get("session", {}).get("callback"),
        "trace_write": bindings.get("trace_write") or bindings.get("mux_write"),
    }


def trace_admin_capacity(records: tuple[bytes, bytes]) -> int:
    return max(map(len, records)) + 4  # CRC-32 trailer


def _canonical_protocol(obj: Any) -> bytes:
    """Canonical protocol JSON has no trailing whitespace or BOM."""
    return canonical(obj).removesuffix(b"\n")


def trace_records(project: dict[str, Any], profile: str) -> tuple[bytes, bytes]:
    """Render immutable canonical protocol metadata and dictionary records."""
    settings = project["profiles"][profile]
    trace = settings.get("trace", {})
    enabled = bool(settings["enabled"] and trace.get("enabled", False))
    symbols = sorted(project.get("symbols", []), key=lambda row: row["id"])
    contexts = sorted(project.get("contexts", []), key=lambda row: row["id"])
    dictionary = {
        "schema_version": 1,
        "generation": 1,
        "symbols": symbols,
        "contexts": contexts,
    }
    dictionary_bytes = _canonical_protocol(dictionary)
    dictionary_hash = hashlib.sha256(dictionary_bytes).hexdigest()
    port = trace.get("port", {})
    clock = trace.get("clock", {})
    features = {
        "trace": enabled,
        "isr": bool(enabled and trace.get("isr", False)),
        "scheduler": bool(enabled and trace.get("scheduler", False)),
        "flow": bool(enabled and trace.get("flow", False)),
    }
    metadata = {
        "schema_version": 1,
        "wire_version": 2,
        "event_schema_version": 1,
        "device_id": project.get("project_name", "Kymo"),
        "build_id": hashlib.sha256(canonical({"project": project, "profile": profile})).hexdigest(),
        "dictionary_hash": dictionary_hash,
        "core_count": int(port.get("core_count", 1)),
        "clock": {
            "clock_domain_id": int(clock.get("domain_id", 0)),
            "frequency_hz": str(clock.get("frequency_hz", 1000000)),
            "counter_bits": int(clock.get("counter_bits", 64)),
            "smp_synchronized": bool(clock.get("smp_synchronized", False)),
            "reset_rule": clock.get("reset_rule", "new_session"),
        },
        "features": features,
        "port_capabilities": {
            "kind": port.get("kind", "generic"),
            "scheduler_complete": bool(port.get("scheduler_complete", False)),
            "isr_complete": bool(port.get("isr_complete", False)),
            "core_state_complete": bool(port.get("core_state_complete", False)),
        },
    }
    return _canonical_protocol(metadata), dictionary_bytes
