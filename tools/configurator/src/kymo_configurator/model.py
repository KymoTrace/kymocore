# SPDX-License-Identifier: MIT
"""Small JSON model helpers. The source document remains a plain versioned dict."""
from __future__ import annotations

import copy
import json
from pathlib import Path
from typing import Any

FEATURES = ("runtime", "x", "z", "timestamp", "crc", "decoder", "cpp")
# Added after schema 2 shipped: absent means off, so existing projects stay valid.
OPTIONAL_FEATURES = ("parameters", "watch", "memory", "crc_table")
TRACE_FEATURES = ("enabled", "isr", "scheduler", "flow")
GENERATOR_VERSION = "0.2.0"
SCHEMA_VERSION = 2
CORE_CONFIG_API_VERSION = 2


def default_profile(enabled: bool, preset: str = "minimal") -> dict[str, Any]:
    """Return one profile with the given preset applied."""
    from .presets import apply_preset
    profile = {
        "enabled": enabled,
        "preset": preset,
        "features": {key: False for key in FEATURES + OPTIONAL_FEATURES},
        "trace": {
            "enabled": False, "isr": False, "scheduler": False, "flow": False,
            "port": {"kind": "generic", "core_count": 1},
            "clock": {"frequency_hz": 1000000, "counter_bits": 64, "domain_id": 1,
                      "smp_synchronized": False, "reset_rule": "new_session"},
            "session": {"callback": "kymo_trace_session_id"},
            "buffer": {"events_per_core": 16, "max_contexts": 16, "max_depth": 32,
                       "max_isr_nesting": 4},
            "transport": {"mode": "dedicated_trace", "max_frame_bytes": 128,
                          "drain_events": 16, "drain_bytes": 256,
                          "max_flush_latency_ms": 10},
            "bindings": {},
        },
    }
    if preset != "custom":
        apply_preset(profile, preset)
    return profile


def default_project(name: str = "KymoProject") -> dict[str, Any]:
    return {
        "schema_version": SCHEMA_VERSION,
        "project_name": name,
        "core_config_api_version": CORE_CONFIG_API_VERSION,
        "profiles": {
            "development": default_profile(True),
            "production": default_profile(False),
        },
        "active_profile": "development",
        "channels": [{"id": 0, "name": "Measurement", "period_ms": 20, "x": False, "z": False, "timestamp": False}],
        "bindings": {"clock": "kymo_port_clock_ms", "sample": "kymo_port_sample", "write": "kymo_port_write", "busy": "kymo_port_busy", "service": "kymo_port_service", "write_borrows_buffer": True},
        "port": {"kind": "generic", "capabilities_verified": False},
        "build": {"system": "gcc", "output_dir": "generated/kymo", "include_dir": "include", "source_dir": "src"},
        "resources": {"ram_budget_bytes": None, "transport_bytes_per_second": None},
        "symbols": [], "contexts": [],
    }


def load_project(path: str | Path) -> dict[str, Any]:
    try:
        doc = json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"Cannot read project {path}: {exc}") from exc
    if not isinstance(doc, dict):
        raise ValueError("Project root must be a JSON object")
    return doc


def save_project(path: str | Path, doc: dict[str, Any]) -> None:
    target = Path(path)
    target.parent.mkdir(parents=True, exist_ok=True)
    data = json.dumps(doc, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    temp = target.with_suffix(target.suffix + ".tmp")
    temp.write_text(data, encoding="utf-8", newline="\n")
    temp.replace(target)
