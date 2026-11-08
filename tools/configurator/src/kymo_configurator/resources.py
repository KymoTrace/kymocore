# SPDX-License-Identifier: MIT
"""Rough RAM estimate of the KymoCore parts a profile compiles in.

The figures are sizeof() of the KymoCore state objects measured on Cortex-M4
(arm-none-eabi-gcc 12.3, -Os), turned into formulas over the configured sizes. They
leave out the application's own buffers and stacks and are an estimate, not a link
map; 8-bit or 64-bit targets differ.
"""
from __future__ import annotations

from typing import Any

from .resolution import resolve_profile
from .watch_config import watch_settings


def estimate_ram(project: dict[str, Any], profile: str) -> dict[str, int]:
    """Bytes per compiled-in part; empty when the profile is disabled."""
    resolved = resolve_profile(project, profile)
    features, trace = resolved["effective_features"], resolved["requested_trace"]
    parts: dict[str, int] = {}
    if not resolved["enabled"]:
        return parts
    channels = max(1, len(project.get("channels", [])))
    if features.get("runtime"):
        parts["runtime"] = 72 + 4 * channels           # KymoContext: 136 B with 16 channels
    if features.get("parameters") or features.get("watch"):
        parts["control"] = 32                          # KymoControlRx
    if features.get("parameters"):
        parts["parameters"] = 64                       # KymoParamReceiver
    if features.get("watch"):
        parts["watch"] = 96 + 16 * int(watch_settings(project)["slots"])  # 224 B with 8 slots
    if features.get("memory"):
        parts["memory"] = 44                           # KymoMemory
    if resolved["effective_trace"]["enabled"]:
        buffer = trace.get("buffer", {}) if isinstance(trace.get("buffer", {}), dict) else {}
        port = trace.get("port", {}) if isinstance(trace.get("port", {}), dict) else {}
        events = int(buffer.get("events_per_core", 64)) * 32 * int(port.get("core_count", 1))
        contexts = int(buffer.get("max_contexts", 16)) * (2 * int(buffer.get("max_depth", 32)) + 24)
        parts["trace"] = events + contexts + 8         # 3,464 B with the defaults
        parts["mux"] = 848                             # KymoMux
    return parts
