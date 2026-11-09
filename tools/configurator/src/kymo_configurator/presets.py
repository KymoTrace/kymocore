# SPDX-License-Identifier: MIT
"""Use-case presets that mirror the KymoCore 7.0 KYMO_PROFILE values.

A preset only fills feature defaults of one configurator profile; every value can
be changed afterwards (the profile then counts as "custom"). Qt-free.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any
from .trace_config import trace_callbacks


@dataclass(frozen=True)
class Preset:
    """One selectable starting point."""
    key: str
    title: str
    question: str
    description: str
    core_profile: str
    features: dict[str, bool] = field(default_factory=dict)
    trace: dict[str, bool] = field(default_factory=dict)


_BASE = {"runtime": True, "x": False, "z": False, "timestamp": False,
         "crc": False, "decoder": False, "cpp": False, "parameters": False, "watch": False,
         "memory": False, "crc_table": False}
_NO_TRACE = {"enabled": False, "isr": False, "scheduler": False, "flow": False}

PRESETS: dict[str, Preset] = {
    "minimal": Preset(
        "minimal", "Values over time",
        "Draw single measurements such as temperature, current or speed as curves over time",
        "One value per channel, as small as possible. Matches KYMO_PROFILE_MINIMAL.",
        "KYMO_PROFILE_MINIMAL", dict(_BASE), dict(_NO_TRACE)),
    "xyz": Preset(
        "xyz", "X/Y or X/Y/Z points",
        "Draw a value over something other than time, e.g. a plotter head position as X/Y, "
        "a 3D position as X/Y/Z or a value over an angle",
        "Enables X, Z and timestamps. Matches KYMO_PROFILE_XYZ.",
        "KYMO_PROFILE_XYZ", {**_BASE, "x": True, "z": True, "timestamp": True},
        dict(_NO_TRACE)),
    "trace": Preset(
        "trace", "Execution tracing",
        "Record which functions, interrupts and flows run when (timing analysis)",
        "XYZ telemetry plus trace recorder with ISR and flow events. Scheduler events "
        "additionally need a verified target port. Corresponds to KYMO_PROFILE_TRACE.",
        "KYMO_PROFILE_TRACE", {**_BASE, "x": True, "z": True, "timestamp": True},
        {"enabled": True, "isr": True, "scheduler": False, "flow": True}),
}

CUSTOM = "custom"

TRACE_DEFAULT_BINDINGS = {"clock": "kymo_port_trace_clock", "trace_write": "kymo_port_trace_write",
                          "session_id": "kymo_trace_session_id"}
"""Function names suggested when tracing is chosen; implemented in the port file."""


def _suggest_trace_bindings(trace: dict[str, Any]) -> None:
    callbacks = trace_callbacks(trace)
    bindings = trace.setdefault("bindings", {})
    for key, name in TRACE_DEFAULT_BINDINGS.items():
        if not callbacks.get(key):
            bindings.setdefault(key, name)


def apply_preset(profile: dict[str, Any], key: str) -> None:
    """Overwrite the feature and trace switches of a profile with a preset."""
    preset = PRESETS[key]
    profile.setdefault("features", {}).update(preset.features)
    trace = profile.setdefault("trace", {})
    trace.update(preset.trace)
    if "stack_pointer" in trace:
        trace["stack_pointer"] = False
    if preset.trace.get("enabled"):
        _suggest_trace_bindings(trace)
    profile["preset"] = key


_XYZ_KEYS = ("x", "z", "timestamp")


def goals(profile: dict[str, Any]) -> tuple[bool, bool]:
    """Return which goals beyond live values are selected: (XYZ with time, tracing)."""
    features = profile.get("features", {})
    trace = profile.get("trace", {})
    return (any(bool(features.get(k)) for k in _XYZ_KEYS), bool(trace.get("enabled", False)))


def apply_goals(profile: dict[str, Any], xyz: bool, trace: bool) -> None:
    """Combine the goals freely; X/Y points send values, so they switch the runtime on.

    Only the switches owned by the goals change, so other features chosen by
    hand (for example CRC) are kept. The profile then reports the preset it
    matches, or "custom" (for example tracing without XYZ).
    """
    features = profile.setdefault("features", {})
    had_xyz, had_trace = goals(profile)
    if xyz != had_xyz:
        if xyz and not values_selected(profile):
            features["runtime"] = True
        for key in _XYZ_KEYS:
            features[key] = xyz
    if trace != had_trace:
        settings = profile.setdefault("trace", {})
        settings["enabled"] = trace
        if trace:
            if not any(trace_callbacks(settings).get(key) for key in ("clock", "trace_write")):
                settings.update(PRESETS["trace"].trace)
            _suggest_trace_bindings(settings)
    profile["preset"] = matching_preset(profile)


GOAL_FEATURES = ("parameters", "watch", "memory")
"""Features offered as their own goal tiles in guided step 1 (and therefore not again in step 2)."""


def values_selected(profile: dict[str, Any]) -> bool:
    """'Values over time' is chosen while KymoCore sends values (runtime or C++ sender)."""
    features = profile.get("features", {})
    return bool(features.get("runtime") or features.get("cpp"))


def set_values_goal(profile: dict[str, Any], on: bool) -> None:
    """Switch 'Values over time'; without it there is nothing to send X/Y points with."""
    features = profile.setdefault("features", {})
    if on:
        features["runtime"] = True
    else:
        for key in ("runtime", "cpp") + _XYZ_KEYS:
            features[key] = False
    profile["preset"] = matching_preset(profile)


def goal_selected(profile: dict[str, Any], goal: str) -> bool:
    """Whether the step-1 goal is chosen; "all" always is."""
    xyz, trace = goals(profile)
    chosen = {"values": values_selected(profile), "xyz": xyz, "trace": trace}
    chosen.update({key: bool(profile.get("features", {}).get(key)) for key in GOAL_FEATURES})
    return chosen.get(goal, True)


def set_goal_feature(profile: dict[str, Any], key: str, on: bool) -> None:
    """Switch one goal feature; parameters and live variables need the frame decoder."""
    features = profile.setdefault("features", {})
    features[key] = on
    if on and key in ("parameters", "watch"):
        features["decoder"] = True
    profile["preset"] = matching_preset(profile)


def matching_preset(profile: dict[str, Any]) -> str:
    """Return the preset whose switches equal the profile, else "custom"."""
    features = profile.get("features", {})
    trace = profile.get("trace", {})
    if (not isinstance(features, dict) or not isinstance(trace, dict)
            or (trace.get("enabled") and trace.get("stack_pointer"))):
        return CUSTOM
    result = CUSTOM
    for key, preset in PRESETS.items():
        if (all(features.get(k, False) is v for k, v in preset.features.items()) and
                all(bool(trace.get(k, False) and (k == "enabled" or trace.get("enabled"))) == v
                    for k, v in preset.trace.items())):
            result = key
            break
    return result
