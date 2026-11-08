# SPDX-License-Identifier: MIT
"""Declarative description of every configurable setting.

The catalog is the single source for both GUI modes: the professional editor
renders every section and field, the guided mode walks through the sections as
steps and hides fields marked ``advanced``. Each field explains in plain words
what it does and names the C macro or binding it affects, so users can see what
the generated configuration will contain.

New features (for example a future back channel) add one ``Section`` with
``register_section`` -- plus optional widgets, validators and renderers through
the matching ``register_*`` hooks -- and appear in both modes automatically.
This module is Qt-free.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Callable

from .model import FEATURES
from .presets import CUSTOM, PRESETS, apply_preset, matching_preset
from .i18n import tr
from .resolution import resolve_profile
from .watch_config import WATCH_DEFAULTS, apply_symbol_preset

PROFILE = "@profile"
"""Path token that stands for ``doc["profiles"][<active profile>]``."""


class View:
    """Read-only helper bound to one document and one active profile."""

    def __init__(self, doc: dict[str, Any], profile: str):
        self.doc = doc
        self.profile_name = profile

    @property
    def profile(self) -> dict[str, Any]:
        profiles = self.doc.get("profiles", {})
        return profiles.get(self.profile_name, {}) if isinstance(profiles, dict) else {}

    def resolved(self) -> dict[str, Any]:
        try:
            return resolve_profile(self.doc, self.profile_name)
        except (KeyError, TypeError, AttributeError):
            return {"enabled": False, "effective_features": {k: False for k in FEATURES},
                    "effective_trace": {"enabled": False}}

    @property
    def enabled(self) -> bool:
        return bool(self.profile.get("enabled", False))

    def feature(self, key: str) -> bool:
        """Effective telemetry feature (requested and KYMO_ENABLED)."""
        return bool(self.resolved()["effective_features"].get(key, False))

    def trace(self, key: str = "enabled") -> bool:
        """Effective trace option."""
        return bool(self.resolved()["effective_trace"].get(key, False))

    def get(self, path: tuple[str, ...], default: Any = None) -> Any:
        return get_path(self.doc, path, self.profile_name, default)


def _resolve(path: tuple[str, ...], profile: str) -> list[str]:
    if path and path[0] == PROFILE:
        return ["profiles", profile, *path[1:]]
    return list(path)


def get_path(doc: dict[str, Any], path: tuple[str, ...], profile: str, default: Any = None) -> Any:
    """Read a value; missing or malformed intermediate objects yield ``default``."""
    node: Any = doc
    for part in _resolve(path, profile):
        if not isinstance(node, dict) or part not in node:
            return default
        node = node[part]
    return node


def set_path(doc: dict[str, Any], path: tuple[str, ...], profile: str, value: Any,
             remove_when_empty: bool = False) -> None:
    """Write a value, creating intermediate objects; optionally drop empty values."""
    parts = _resolve(path, profile)
    node = doc
    for part in parts[:-1]:
        child = node.get(part)
        if not isinstance(child, dict):
            child = {}
            node[part] = child
        node = child
    if remove_when_empty and value in ("", None):
        node.pop(parts[-1], None)
    else:
        node[parts[-1]] = value


Predicate = Callable[[View], bool]
ChangeHook = Callable[[dict[str, Any], str, Any, Any], None]


@dataclass(frozen=True)
class Field:
    """One setting.

    ``kind`` is one of ``bool``, ``int``, ``optional_int``, ``choice``,
    ``identifier`` (a C symbol name, empty allowed unless required by validation),
    ``text`` or ``preset``.
    """
    key: str
    path: tuple[str, ...]
    label: str
    help: str
    kind: str = "bool"
    default: Any = None
    choices: tuple[tuple[Any, str], ...] = ()
    minimum: int | None = None
    maximum: int | None = None
    target: str = ""
    """C macro, binding or generated symbol affected by this field."""
    advanced: bool = False
    enabled_when: Predicate | None = None
    requires: str = ""
    """Human-readable reason shown while ``enabled_when`` is false."""
    visible_when: Predicate | None = None
    on_change: ChangeHook | None = None
    placeholder: str = ""
    guided_help: str = ""
    """Beginner wording shown instead of ``help`` in guided mode (says when it is needed)."""
    example: str = ""
    """Code the user writes for this binding; ``{name}`` is replaced by the entered name."""
    need: Predicate | None = None
    """True: shown as required, False: shown as optional, None: no mark."""

    def value(self, view: View) -> Any:
        return view.get(self.path, self.default)

    def is_enabled(self, view: View) -> bool:
        return self.enabled_when is None or bool(self.enabled_when(view))

    def is_visible(self, view: View) -> bool:
        return self.visible_when is None or bool(self.visible_when(view))

    def store(self, doc: dict[str, Any], profile: str, value: Any) -> None:
        """Write a value (with type normalization) and run the change hook."""
        old = get_path(doc, self.path, profile, self.default)
        if self.kind in ("identifier", "text"):
            value = str(value).strip() if value is not None else ""
        if self.kind == "optional_int" and value == "":
            value = None
        set_path(doc, self.path, profile, value,
                 remove_when_empty=self.kind == "identifier")
        if self.on_change is not None and old != value:
            self.on_change(doc, profile, old, value)


@dataclass(frozen=True)
class Group:
    """A titled set of fields inside a section."""
    title: str
    fields: tuple[Field, ...]
    description: str = ""
    advanced: bool = False
    visible_when: Predicate | None = None
    guided_description: str = ""
    """Beginner wording shown instead of ``description`` in guided mode."""


@dataclass(frozen=True)
class Section:
    """One page in the professional editor and one step in guided mode."""
    key: str
    title: str
    description: str
    groups: tuple[Group, ...] = ()
    widgets: tuple[str, ...] = ()
    """Names of custom widgets rendered below the groups (see GUI widget registry)."""
    guided_title: str = ""
    guided_intro: str = ""
    diagnostic_prefixes: tuple[str, ...] = ()
    """Diagnostic paths (``{profile}`` is replaced) that belong to this section."""
    summary: Callable[[View], str] | None = None
    visible_when: Predicate | None = None
    guided_when: Predicate | None = None
    """Show as guided step only when true (defaults to visible_when)."""
    order: int = 100

    def fields(self, guided: bool = False) -> list[Field]:
        result = []
        for group in self.groups:
            if guided and group.advanced:
                continue
            result.extend(f for f in group.fields if not (guided and f.advanced))
        return result

    def is_visible(self, view: View) -> bool:
        return self.visible_when is None or bool(self.visible_when(view))

    def in_guided(self, view: View) -> bool:
        predicate = self.guided_when or self.visible_when
        return predicate is None or bool(predicate(view))

    def owns(self, diagnostic_path: str, profile: str) -> bool:
        return any(diagnostic_path == p.format(profile=profile) or
                   diagnostic_path.startswith(p.format(profile=profile) + ".") or
                   diagnostic_path.startswith(p.format(profile=profile) + "[")
                   for p in self.diagnostic_prefixes)


# --- change hooks ----------------------------------------------------------

def _mark_custom(doc: dict[str, Any], profile: str, old: Any, new: Any) -> None:
    """Editing a switch by hand turns the profile into a custom selection."""
    current = get_path(doc, (PROFILE,), profile, {})
    if isinstance(current, dict):
        current["preset"] = matching_preset(current)


def _apply_preset(doc: dict[str, Any], profile: str, old: Any, new: Any) -> None:
    current = get_path(doc, (PROFILE,), profile, None)
    if isinstance(current, dict) and new in PRESETS:
        apply_preset(current, new)


def _port_changed(doc: dict[str, Any], profile: str, old: Any, new: Any) -> None:
    """A different port invalidates earlier capability verification."""
    set_path(doc, ("port", "capabilities_verified"), profile, False)
    set_path(doc, (PROFILE, "trace", "port", "kind"), profile, new)


# --- predicates ------------------------------------------------------------

def _on(view: View) -> bool:
    return view.enabled


def _runtime(view: View) -> bool:
    return view.feature("runtime")


def _cpp(view: View) -> bool:
    return view.feature("cpp")


def _cpp_time(view: View) -> bool:
    return view.feature("cpp") and view.feature("timestamp")


def _trace(view: View) -> bool:
    return view.trace("enabled")


def _trace_requested(view: View) -> bool:
    return bool(view.get((PROFILE, "trace", "enabled"), False))


def _runtime_direct_write(view: View) -> bool:
    shared = view.get((PROFILE, "trace", "transport", "mode"), "") == "shared_v2" and view.trace()
    return view.feature("runtime") and not shared


def _never(view: View) -> bool:
    return False


def _busy_needed(view: View) -> bool:
    return _runtime_direct_write(view) and bool(view.get(("bindings", "write_borrows_buffer"), True))


def _trace_busy_needed(view: View) -> bool:
    return view.trace() and bool(view.get((PROFILE, "trace", "transport", "write_borrows_buffer"), False))


def _watch(view: View) -> bool:
    return view.feature("watch")


def _watch_symbols_changed(doc: dict[str, Any], profile: str, old: Any, new: Any) -> None:
    """A symbol preset fills in its linker symbols; 'custom' keeps what was entered."""
    apply_symbol_preset(doc, new)


def _watch_field(key: str, label: str, help_text: str, **extra: Any) -> Field:
    return Field(f"watch.{key}", ("watch", key), label, help_text,
                 default=WATCH_DEFAULTS[key], enabled_when=_watch,
                 requires="Enable live variables first.", **extra)


def _feature(key: str, label: str, help_text: str, macro: str, advanced: bool = False,
             enabled_when: Predicate | None = _on, guided: str = "") -> Field:
    return Field(f"features.{key}", (PROFILE, "features", key), label, help_text,
                 target=macro, advanced=advanced, enabled_when=enabled_when,
                 requires="Enable KymoCore for this profile first.", on_change=_mark_custom,
                 guided_help=guided)


def _binding(key: str, label: str, help_text: str, target: str, advanced: bool = False,
             enabled_when: Predicate | None = _runtime, requires: str = "Needs the telemetry runtime.",
             placeholder: str = "", guided: str = "", example: str = "",
             need: Predicate | None = None) -> Field:
    return Field(f"bindings.{key}", ("bindings", key), label, help_text, kind="identifier",
                 default="", target=target, advanced=advanced, enabled_when=enabled_when,
                 requires=requires, placeholder=placeholder, guided_help=guided, example=example,
                 need=need)


def _trace_binding(key: str, label: str, help_text: str, target: str, advanced: bool = False,
                   guided: str = "", example: str = "", need: Predicate | None = None) -> Field:
    return Field(f"trace.bindings.{key}", (PROFILE, "trace", "bindings", key), label, help_text,
                 kind="identifier", default="", target=target, advanced=advanced,
                 enabled_when=_trace_requested, requires="Enable trace recording first.",
                 guided_help=guided, example=example, need=need)


def _trace_int(key: tuple[str, ...], label: str, help_text: str, low: int, high: int, default: int,
               target: str = "", advanced: bool = False, guided: str = "") -> Field:
    return Field("trace." + ".".join(key), (PROFILE, "trace", *key), label, help_text, kind="int",
                 default=default, minimum=low, maximum=high, target=target, advanced=advanced,
                 enabled_when=_trace_requested, requires="Enable trace recording first.",
                 guided_help=guided)


# --- summaries -------------------------------------------------------------

def _summary_project(view: View) -> str:
    preset = view.profile.get("preset", CUSTOM)
    title = tr(PRESETS[preset].title) if preset in PRESETS else tr("Custom selection")
    state = tr("enabled") if view.enabled else tr("compiled out (KYMO_ENABLED=0)")
    return tr("Profile '{name}': {title}, {state}", name=view.profile_name, title=title, state=state)


def _summary_features(view: View) -> str:
    on = [k for k in ("x", "z", "timestamp", "crc", "decoder", "parameters", "watch", "cpp") if view.feature(k)]
    api = tr("Init/Main runtime") if view.feature("runtime") else tr("no runtime")
    return api + (", " + ", ".join(on) if on else ", " + tr("scalar values only"))


def _summary_channels(view: View) -> str:
    channels = view.doc.get("channels", [])
    if not isinstance(channels, list):
        return tr("invalid channel list")
    rate = 0.0
    traffic = 0.0
    for ch in channels:
        period = ch.get("period_ms") if isinstance(ch, dict) else None
        if type(period) is int and period > 0:
            size = 9 + 4 * sum(bool(ch.get(k)) for k in ("x", "z", "timestamp"))
            rate += 1000.0 / period
            traffic += 1000.0 / period * size
    return tr("{count} channel(s), ~{rate} frames/s, ~{traffic} bytes/s",
              count=len(channels), rate=f"{rate:.0f}", traffic=f"{traffic:.0f}")


def _summary_callbacks(view: View) -> str:
    b = view.doc.get("bindings", {}) if isinstance(view.doc.get("bindings"), dict) else {}
    missing = [k for k in ("clock", "sample", "write") if not b.get(k)]
    return tr("all required callbacks named") if not missing else tr("missing: ") + ", ".join(missing)


def _summary_trace(view: View) -> str:
    if not view.trace():
        return tr("off")
    events = [k for k in ("isr", "scheduler", "flow") if view.trace(k)]
    mode = view.get((PROFILE, "trace", "transport", "mode"), "dedicated_trace")
    return tr("recording") + (" + " + ", ".join(events) if events else "") + f", {mode}"


def _summary_build(view: View) -> str:
    build = view.doc.get("build", {}) if isinstance(view.doc.get("build"), dict) else {}
    return f"{build.get('system', 'gcc')} → {build.get('output_dir', 'generated/kymo')}"


_EVENTS_CHOICES = tuple((value, str(value)) for value in (16, 32, 64, 128, 256, 512, 1024, 2048, 4096))

SECTIONS: list[Section] = [
    Section(
        "project", "Goal & profile",
        "Name the project and choose what KymoCore should do in the selected build profile. "
        "A profile is one named build variant (for example development and production).",
        groups=(
            Group("Project", (
                Field("project_name", ("project_name",), "Project name",
                      "Used for generated metadata and as the trace device name.", kind="text",
                      default="KymoProject",
                      guided_help="Any name you like, e.g. 'Plant monitor'. KymoStudio shows it so you "
                                  "recognise your device."),
            )),
            Group("This profile", (
                Field("profile.enabled", (PROFILE, "enabled"), "Use KymoCore in this profile",
                      "Off compiles the whole library out (for example in production). All other "
                      "settings are kept and become active again when switched back on.",
                      target="KYMO_ENABLED",
                      guided_help="Leave this ticked. Untick it only for a build in which KymoCore should "
                                  "disappear completely, e.g. the finished product. Your settings are kept."),
                Field("profile.preset", (PROFILE, "preset"), "What do you want to do?",
                      "Choosing a goal sets the matching features in one step. You can change "
                      "each feature afterwards; the profile then shows as custom.",
                      kind="preset", default=CUSTOM,
                      choices=tuple((p.key, p.title) for p in PRESETS.values()) + ((CUSTOM, "Custom selection"),),
                      target="KYMO_PROFILE", enabled_when=_on,
                      requires="Enable KymoCore for this profile first.", on_change=_apply_preset,
                      guided_help="Tick everything you want to use; the options combine freely. Hover "
                                  "over an option for examples. "
                                  "This sets the matching features for you, and the next steps show "
                                  "what was chosen."),
            ), guided_description="A profile is a named set of settings for one firmware build, e.g. "
                                  "'development' with all measurements and 'production' with KymoCore "
                                  "switched off. Most projects need only one. The 'Build profile' selector "
                                  "at the top switches between profiles, 'New profile…' adds one."),
        ),
        guided_title="What do you want to achieve?",
        guided_intro="KymoCore sends measurements from your microcontroller to KymoStudio on your PC, "
                     "where they are drawn as live curves. This wizard asks a few questions and then "
                     "writes the matching configuration files for your firmware. Nothing is final: "
                     "every answer can be changed later.",
        diagnostic_prefixes=("project_name", "profiles.{profile}.enabled", "profiles.{profile}.preset",
                             "active_profile", "schema_version", "core_config_api_version", "profiles"),
        summary=_summary_project, order=10),
    Section(
        "telemetry", "Telemetry features",
        "Which parts of KymoCore are compiled in and which fields a data frame may carry. "
        "Disabled parts cost no code.",
        groups=(
            Group("Programming interface", (
                _feature("runtime", "Cyclic runtime (Kymo_Init / Kymo_Main)",
                         "KymoCore samples your channels on schedule; recommended for most projects.",
                         "KYMO_ENABLE_RUNTIME",
                         guided="Leave ticked unless you know otherwise. KymoCore then reads your "
                                "measurements by itself at a fixed rhythm (e.g. every 20 ms) and sends them. "
                                "Your program only calls Kymo_Init() once and Kymo_Main() in its main loop."),
                _feature("cpp", "C++ push sender (KymoSender)",
                         "Send single values whenever your code decides, in addition to or instead of "
                         "the cyclic runtime.", "KYMO_ENABLE_CPP",
                         guided="Tick this only if you want to send a value at a moment you choose, e.g. "
                                "right after a sensor reading. Needs C++ (Arduino is C++). Not needed "
                                "for normal cyclic sending."),
            ), guided_description="How your program hands its values to KymoCore."),
            Group("Data per sample", (
                _feature("x", "X coordinate", "Allows channels to send an X value, e.g. position or angle.",
                         "KYMO_ENABLE_X",
                         guided="Tick if a signal is a point instead of a single number, e.g. the X/Y "
                                "position of a plotter head, or a value drawn over an angle instead of over time."),
                _feature("z", "Z coordinate", "Allows channels to send a Z value for 3D data.",
                         "KYMO_ENABLE_Z",
                         guided="Tick for 3D points (X, Y and Z), e.g. a position in space."),
                _feature("timestamp", "Timestamp", "Adds the time since start to each sample (4 bytes).",
                         "KYMO_ENABLE_TIMESTAMP",
                         guided="Tick if KymoStudio must know exactly when each value was measured, e.g. "
                                "for precise timing. Without it, KymoStudio uses the arrival time, which is "
                                "fine for most live curves. Costs 4 extra bytes per value."),
            ), description="Enabling a field allows it; each channel then chooses whether it uses it.",
               guided_description="By default every measurement is one number, drawn over time. Tick "
                                  "an extra field only if some of your signals need it; each channel can "
                                  "then decide in the next step whether it uses it."),
            Group("Integrity and decoding", (
                _feature("crc", "CRC checksum on sent frames",
                         "Detects transmission errors on noisy links. Older receivers require it.",
                         "KYMO_ENABLE_CRC",
                         guided="Tick if the connection can corrupt data (long cables, radio, motors "
                                "nearby). Broken frames are then dropped instead of drawn wrong. Over a "
                                "short USB cable it is usually not needed."),
                _feature("decoder", "Frame decoder on the MCU",
                         "Only needed when the microcontroller itself must read Kymo frames.",
                         "KYMO_ENABLE_DECODER", advanced=True),
                _feature("parameters", "Parameters from KymoStudio (back channel)",
                         "Lets KymoStudio change registered variables at run time; needs the frame decoder.",
                         "KYMO_ENABLE_PARAMETERS",
                         guided="Tick if you want to tune values such as controller gains or a setpoint "
                                "from KymoStudio while measuring. You list them in your code with "
                                "KYMO_PARAM; the firmware checks every value. Also tick the frame decoder."),
                _feature("watch", "Live variables from the ELF file",
                         "Lets KymoStudio sample global variables by address inside the memory ranges "
                         "you allow; needs the frame decoder.",
                         "KYMO_ENABLE_WATCH",
                         guided="Tick if you want to pick variables in KymoStudio from your firmware's "
                                "ELF file and watch them as curves without adding code for each one. "
                                "Your firmware lists which memory may be read; nothing is ever written. "
                                "Also tick the frame decoder."),
            )),
            Group("Live variables", (
                _watch_field("symbols", "Linker symbols",
                             "Fills in the symbols that enclose .data and .bss for common toolchains.",
                             kind="choice", on_change=_watch_symbols_changed,
                             choices=(("esp32", "ESP32 (_data_start .. _bss_end)"),
                                      ("stm32", "STM32CubeIDE (_sdata .. _ebss)"),
                                      ("cmsis", "CMSIS GCC, RP2040, nRF (__data_start__ .. __bss_end__)"),
                                      ("custom", "Custom")),
                             guided_help="Choose your chip family. The firmware then allows KymoStudio "
                                         "to read exactly your global variables and nothing else."),
                _watch_field("range_start", "Readable range: start symbol",
                             "Linker symbol at the first readable byte.", kind="identifier"),
                _watch_field("range_end", "Readable range: end symbol",
                             "Linker symbol just past the last readable byte.", kind="identifier"),
                _watch_field("range2_start", "Second range: start symbol",
                             "Optional, e.g. external RAM.", kind="identifier", advanced=True),
                _watch_field("range2_end", "Second range: end symbol",
                             "Optional, e.g. external RAM.", kind="identifier", advanced=True),
                _watch_field("build_id", "Build ID callback",
                             "Your function size_t name(const uint8_t **id) that points at the build ID "
                             "and returns its length. Empty: KymoStudio cannot check the ELF file.",
                             kind="identifier", placeholder="kymo_port_build_id",
                             example="size_t {name}(const uint8_t **id) { *id = build_id; return 16u; }"),
                _watch_field("slots", "Slots", "Variables sampled at the same time; 16 bytes RAM each.",
                             kind="int", minimum=1, maximum=64, target="KYMO_MAX_WATCHES", advanced=True),
                _watch_field("min_period_ms", "Shortest period (ms)",
                             "Requests below this period are rejected.", kind="int", minimum=1,
                             maximum=60000, target="KYMO_WATCH_MIN_PERIOD_MS", advanced=True),
            ), description="Generates kymo_generated_watch.cpp; call kymo_generated_watch_init() once "
                           "at start-up.", visible_when=_watch),
        ),
        guided_title="Which extras do you need?",
        guided_intro="Here you choose which building blocks of KymoCore end up on your "
                     "microcontroller. Your answer in step 1 has already ticked what you need. "
                     "Not sure? Leave everything as it is and click Next. Unticked parts use no memory.",
        diagnostic_prefixes=("profiles.{profile}.features",),
        summary=_summary_features, visible_when=_on, order=20),
    Section(
        "channels", "Channels",
        "Each channel is one signal in KymoStudio. It has an ID (0..255), a sampling period and "
        "the optional fields it sends. KYMO_MAX_CHANNELS is sized automatically.",
        widgets=("channels",),
        guided_title="What do you want to measure?",
        guided_intro="Add one row per signal. The period is how often it is sampled "
                     "(20 ms = 50 times per second). The estimate below shows the resulting traffic.",
        diagnostic_prefixes=("channels",),
        summary=_summary_channels, visible_when=_runtime, order=30),
    Section(
        "callbacks", "Callbacks & transport",
        "KymoCore never touches hardware itself. You name small functions in your own code; "
        "the generated configuration calls them. Their implementations stay in your project.",
        groups=(
            Group("Required functions", (
                _binding("clock", "Time function",
                         "Enter the name of your function that returns the milliseconds since start.",
                         "uint32_t clock(void *user)", placeholder="kymo_port_clock_ms", need=_runtime,
                         example="uint32_t {name}(void *user)\n{\n    return millis();\n}"),
                _binding("sample", "Measurement function",
                         "Enter the name of your function that reads a measurement. KymoCore passes the "
                         "channel ID; your function writes the value into out->value and returns 1 "
                         "(or 0 to skip this time).",
                         "uint8_t sample(void *user, uint8_t id, KymoSample *out)",
                         placeholder="kymo_port_sample", need=_runtime,
                         example="uint8_t {name}(void *user, uint8_t id, KymoSample *out)\n{\n"
                                 "    uint8_t ok = 0;\n    if (id == 0) {    /* channel ID from the channel table */\n"
                                 "        out->value = analogRead(A0);\n        ok = 1;\n    }\n"
                                 "    return ok;\n}"),
                _binding("write", "Send function",
                         "Enter the name of your function that sends bytes to the PC and returns how many "
                         "it accepted.",
                         "uint8_t write(void *user, const uint8_t *bytes, uint8_t length)",
                         enabled_when=_runtime_direct_write,
                         requires="Not used: telemetry goes through the shared trace link.",
                         placeholder="kymo_port_write", need=_runtime_direct_write,
                         example="uint8_t {name}(void *user, const uint8_t *bytes, uint8_t length)\n{\n"
                                 "    return (uint8_t)Serial.write(bytes, length);\n}"),
            ), guided_description="KymoCore does not know your hardware. You write a few small functions "
                                  "and enter their names here; the generated files call them. Not written "
                                  "yet? Use the grey suggested names: the 'init-port' command creates a "
                                  "starter file containing exactly these functions."),
            Group("Transport behaviour", (
                Field("bindings.write_borrows_buffer", ("bindings", "write_borrows_buffer"),
                      "Driver keeps the bytes after write() returns (DMA/USB)",
                      "Then a busy function is required so KymoCore does not overwrite the frame "
                      "while it is still being sent.", default=True, enabled_when=_runtime,
                      requires="Needs the telemetry runtime.", target="KymoConfig.busy",
                      guided_help="Tick if your driver sends in the background (DMA, many USB stacks) and "
                                  "still uses the bytes after your send function returned. Then the 'Busy "
                                  "check' function below is needed too. For a plain Serial.write() untick it."),
                _binding("busy", "Busy check",
                         "Enter the name of your function that returns 1 while the driver is still sending "
                         "the last data, otherwise 0. Only needed when the box above is ticked.",
                         "uint8_t busy(void *user)", placeholder="kymo_port_busy", need=_busy_needed,
                         example="uint8_t {name}(void *user)\n{\n    return uart_dma_busy ? 1 : 0;\n}"),
                _binding("service", "Service function",
                         "Name of a function KymoCore calls on every cycle, e.g. to keep a WiFi connection "
                         "alive. Leave empty if you do not need it.",
                         "void service(void *user)", placeholder="kymo_port_service", need=_never,
                         example="void {name}(void *user)\n{\n    wifi_client.poll();\n}"),
            ), description="How the bytes leave your microcontroller."),
            Group("Extra data for your functions", (
                _binding("clock_user", "Object for the time function",
                         "Name of a global object that KymoCore passes to your time function as user.",
                         "KymoConfig.clock_user", advanced=True, need=_never, placeholder="my_timer",
                         example="static MyTimer {name};    /* your function receives &{name} as user */"),
                _binding("sample_user", "Object for the measurement function",
                         "Name of a global object that KymoCore passes to your measurement function as user.",
                         "KymoConfig.sample_user", advanced=True, need=_never, placeholder="my_sensors",
                         example="static MySensors {name};    /* your function receives &{name} as user */"),
                _binding("transport_user", "Object for the send functions",
                         "Name of a global object that KymoCore passes to your send, busy and service "
                         "functions as user.",
                         "KymoConfig.transport_user", advanced=True, need=_never, placeholder="my_uart",
                         example="static MyUart {name};    /* your functions receive &{name} as user */"),
            ), advanced=True,
               description="Optional, usually left empty. Every function above receives a parameter "
                           "void *user. If a function needs to know which object to work with, e.g. which "
                           "of two serial ports, enter that object's name here and KymoCore passes a "
                           "pointer to it as user. If your functions simply use globals such as Serial, "
                           "leave everything empty."),
            Group("C++ push sender", (
                _binding("cpp_stream", "KymoStream object",
                         "Name of your object derived from KymoStream that the C++ sender writes to.",
                         "KymoStream &", enabled_when=_cpp, requires="Enable the C++ push sender.",
                         need=_cpp,
                         example="class SerialStream : public KymoStream {\npublic:\n"
                                 "    size_t write(const uint8_t *data, size_t length) override\n    {\n"
                                 "        return Serial.write(data, length);\n    }\n};\n\nSerialStream {name};"),
                _binding("cpp_clock", "Millisecond clock",
                         "Name of your function without arguments that returns milliseconds; used for "
                         "C++ timestamps.",
                         "uint32_t clock(void)", enabled_when=_cpp_time,
                         requires="Needs the C++ sender and timestamps.", need=_cpp_time,
                         example="uint32_t {name}(void)\n{\n    return millis();\n}"),
                Field("bindings.cpp_timestamp_default", ("bindings", "cpp_timestamp_default"),
                      "Send timestamps by default", "Initial value of KymoSender::setTimestampEnabled.",
                      default=False, enabled_when=_cpp_time, requires="Needs the C++ sender and timestamps.",
                      target="KymoSender::begin"),
            ), visible_when=_cpp),
        ),
        guided_title="How does KymoCore reach your hardware?",
        guided_intro="KymoCore runs on any microcontroller because it never talks to the hardware "
                     "itself. Instead it calls a few functions from your own program: one for the time, "
                     "one that reads a measurement and one that sends bytes. Here you only enter their names.",
        diagnostic_prefixes=("bindings",),
        summary=_summary_callbacks, visible_when=lambda v: v.feature("runtime") or v.feature("cpp"),
        order=40),
    Section(
        "trace", "Execution tracing",
        "Records function scopes, markers, interrupts and flows with timestamps and sends them "
        "to KymoStudio for timing analysis.",
        groups=(
            Group("What is recorded", (
                Field("trace.enabled", (PROFILE, "trace", "enabled"), "Trace recording",
                      "Compiles the recorder, the KYMO_TRACE_* macros and the transport mux.",
                      guided_help="Leave ticked to record when your functions run. You mark the "
                                  "interesting places in your code with KYMO_TRACE_* macros.",
                      target="KYMO_ENABLE_TRACE", enabled_when=_on,
                      requires="Enable KymoCore for this profile first.", on_change=_mark_custom),
                Field("trace.isr", (PROFILE, "trace", "isr"), "Interrupts",
                      "Records interrupt entry and exit.", target="KYMO_ENABLE_TRACE_ISR",
                      guided_help="Tick to see when interrupt handlers run and how long they take.",
                      enabled_when=_trace_requested, requires="Enable trace recording first.",
                      on_change=_mark_custom),
                Field("trace.scheduler", (PROFILE, "trace", "scheduler"), "Task switches",
                      "Records RTOS task switches. Needs interrupt tracing and a verified port.",
                      guided_help="Only for RTOS projects (e.g. FreeRTOS): shows which task runs when. "
                                  "Leave unticked if you do not use an RTOS.",
                      target="KYMO_ENABLE_TRACE_SCHEDULER", enabled_when=_trace_requested,
                      requires="Enable trace recording first.", on_change=_mark_custom),
                Field("trace.flow", (PROFILE, "trace", "flow"), "Flows",
                      "Correlates send/receive pairs across tasks.", target="KYMO_ENABLE_TRACE_FLOW",
                      guided_help="Draws arrows from where data is sent to where it is received, e.g. "
                                  "between an interrupt and a task.",
                      enabled_when=_trace_requested, requires="Enable trace recording first.",
                      on_change=_mark_custom),
            )),
            Group("Target", (
                Field("port.kind", ("port", "kind"), "Target port",
                      "Describes the platform integration. Changing it resets capability verification.",
                      guided_help="Your hardware family. Choose 'Generic' if yours is not listed.",
                      kind="choice", default="generic",
                      choices=(("generic", "Generic"), ("esp32_freertos", "ESP32 + FreeRTOS"),
                               ("stm32_baremetal", "STM32 bare metal")),
                      enabled_when=_trace_requested, requires="Enable trace recording first.",
                      on_change=_port_changed),
                _trace_int(("port", "core_count"), "CPU cores", "Number of cores that record events.",
                           1, 2, 1, "KYMO_TRACE_CORE_COUNT",
                           guided="1 for most microcontrollers; 2 for dual-core chips such as the ESP32."),
                _trace_int(("clock", "frequency_hz"), "Trace clock (Hz)",
                           "Tick rate of your trace clock, e.g. 1000000 for microseconds.",
                           1, 2147483647, 1000000,
                           guided="How many times per second your trace clock counts. 1000000 means it "
                                  "counts microseconds, e.g. micros() on Arduino."),
                Field("trace.clock.counter_bits", (PROFILE, "trace", "clock", "counter_bits"),
                      "Clock counter width", "Width of the hardware counter.", kind="choice",
                      default=64, choices=((32, "32 bit"), (64, "64 bit")), advanced=True,
                      enabled_when=_trace_requested, requires="Enable trace recording first."),
                _trace_int(("clock", "domain_id"), "Clock domain ID", "Identifies the clock in KymoStudio.",
                           0, 65535, 1, advanced=True),
                Field("trace.clock.smp_synchronized", (PROFILE, "trace", "clock", "smp_synchronized"),
                      "Clock synchronized across cores", "Set when all cores read the same counter.",
                      advanced=True, enabled_when=_trace_requested, requires="Enable trace recording first."),
            )),
            Group("Buffers", (
                Field("trace.buffer.events_per_core", (PROFILE, "trace", "buffer", "events_per_core"),
                      "Events per core", "Ring size per core; 32 bytes RAM per event.", kind="choice",
                      guided_help="How many events are buffered before they are sent. More events need "
                                  "more RAM (32 bytes each) but lose less during bursts.",
                      default=16, choices=_EVENTS_CHOICES, target="KYMO_TRACE_EVENTS_PER_CORE",
                      enabled_when=_trace_requested, requires="Enable trace recording first."),
                _trace_int(("buffer", "max_contexts"), "Tasks / contexts", "How many tasks or ISRs are tracked.",
                           1, 256, 16, "KYMO_TRACE_MAX_CONTEXTS", advanced=True),
                _trace_int(("buffer", "max_depth"), "Nesting depth", "Deepest nesting of traced scopes.",
                           1, 255, 32, "KYMO_TRACE_MAX_DEPTH", advanced=True),
                _trace_int(("buffer", "max_isr_nesting"), "Interrupt nesting", "Deepest interrupt nesting.",
                           1, 16, 4, "KYMO_TRACE_MAX_ISR_DEPTH", advanced=True),
            )),
            Group("Link", (
                Field("trace.transport.mode", (PROFILE, "trace", "transport", "mode"), "Link usage",
                      "Shared: telemetry frames and trace events use the same link through the mux.",
                      guided_help="Choose 'Shared' if measurements and trace events go over the same "
                                  "cable, e.g. one USB serial port.",
                      kind="choice", default="dedicated_trace",
                      choices=(("dedicated_trace", "Trace only on this link"),
                               ("shared_v2", "Shared with telemetry")),
                      target="KymoMuxConfig.next_data", enabled_when=_trace_requested,
                      requires="Enable trace recording first."),
                _trace_int(("transport", "max_frame_bytes"), "Largest frame (bytes)",
                           "Maximum bytes your link sends at once (64..240).", 64, 240, 128,
                           "KymoMuxConfig.max_frame_size",
                           guided="The default of 128 works for serial and USB."),
                _trace_int(("transport", "drain_events"), "Events per service call",
                           "Upper bound of events sent per cycle.", 1, 4096, 16, advanced=True),
                _trace_int(("transport", "drain_bytes"), "Bytes per service call",
                           "Upper bound of bytes sent per cycle.", 1, 65535, 256, advanced=True),
                _trace_int(("transport", "max_flush_latency_ms"), "Health interval (ms)",
                           "How often a health record is sent.", 1, 2147483647, 10,
                           "KymoMuxConfig.health_period_ticks", advanced=True),
                Field("trace.transport.write_borrows_buffer",
                      (PROFILE, "trace", "transport", "write_borrows_buffer"),
                      "Driver keeps the bytes after write() returns",
                      "Then the transport busy function is required.", default=False,
                      guided_help="Tick only if your driver sends in the background (DMA, USB) and then "
                                  "fill in 'Link busy' below.",
                      enabled_when=_trace_requested, requires="Enable trace recording first."),
            )),
            Group("Trace functions", (
                _trace_binding("clock", "Trace clock", "Returns the free-running tick counter.",
                               "uint64_t clock(void *user)",
                               guided="Name of your function that returns the trace clock count.", need=_trace,
                               example="uint64_t {name}(void *user)\n{\n    return micros();\n}"),
                _trace_binding("session_id", "Session ID", "Returns a new random ID per boot.",
                               "uint64_t session(void)",
                               guided="Name of a function that returns a random number, new on every start, "
                                      "so KymoStudio can tell recordings apart.", need=_trace,
                               example="uint64_t {name}(void)\n{\n"
                                       "    return ((uint64_t)random() << 32) | (uint32_t)random();\n}"),
                _trace_binding("trace_write", "Send trace bytes", "Writes mux frames to the link.",
                               "size_t write(void *user, const uint8_t *bytes, size_t length)",
                               guided="Name of the function that sends the trace bytes to the PC.", need=_trace,
                               example="size_t {name}(void *user, const uint8_t *bytes, size_t length)\n{\n"
                                       "    return Serial.write(bytes, length);\n}"),
                _trace_binding("transport_busy", "Link busy", "Returns nonzero while the driver owns the frame.",
                               "uint8_t busy(void *user)",
                               guided="Only needed with background sending: returns 1 while still sending.",
                               need=_trace_busy_needed,
                               example="uint8_t {name}(void *user)\n{\n    return uart_dma_busy ? 1 : 0;\n}"),
                _trace_binding("core_id", "Current core", "Empty means core 0.",
                               "uint8_t core_id(void *user)", advanced=True, need=_never,
                               example="uint8_t {name}(void *user)\n{\n    return (uint8_t)xPortGetCoreID();\n}"),
                _trace_binding("current_context", "Current task",
                               "Empty means unknown; used as interrupted context.",
                               "uint32_t context(void *user)", advanced=True, need=_never),
                _trace_binding("critical_enter", "Critical section enter",
                               "Needed when several tasks or ISRs record.", "void enter(void *user)",
                               advanced=True, need=_never,
                               example="void {name}(void *user)\n{\n    noInterrupts();\n}"),
                _trace_binding("critical_exit", "Critical section exit",
                               "Must be set together with enter.", "void exit(void *user)", advanced=True,
                               need=_never, example="void {name}(void *user)\n{\n    interrupts();\n}"),
            )),
        ),
        widgets=("symbols", "contexts"),
        guided_title="Configure execution tracing",
        guided_intro="Tracing records when your functions and interrupts run, so KymoStudio can show a "
                     "timeline. It needs a fast clock and a connection to the PC. The lists below give "
                     "your functions (symbols) and tasks (contexts) readable names in KymoStudio.",
        diagnostic_prefixes=("profiles.{profile}.trace", "port", "symbols", "contexts"),
        summary=_summary_trace, visible_when=_on, guided_when=_trace, order=50),
    Section(
        "build", "Build & output",
        "Where the generated, immutable configuration snapshot is written and which build "
        "system helper files are produced.",
        groups=(
            Group("Output", (
                Field("build.system", ("build", "system"), "Build system",
                      "Produces matching include helpers (Makefile, CMake or PlatformIO scripts).",
                      kind="choice", default="gcc",
                      choices=(("gcc", "Plain compiler / Makefile"), ("cmake", "CMake"),
                               ("platformio", "PlatformIO")),
                      guided_help="The tool that builds your firmware. It decides which helper files are "
                                  "created so your build finds the configuration. Using VS Code with "
                                  "PlatformIO? Choose PlatformIO."),
                Field("build.output_dir", ("build", "output_dir"), "Output folder",
                      "Relative to the project file; one subfolder per profile.", kind="text",
                      default="generated/kymo",
                      guided_help="Where the generated files are stored, next to the project file. The "
                                  "default is fine for most projects."),
                Field("build.include_dir", ("build", "include_dir"), "Project include folder",
                      "Your project's include folder (informational).", kind="text", default="include",
                      advanced=True),
                Field("build.source_dir", ("build", "source_dir"), "Project source folder",
                      "Your project's source folder (informational).", kind="text", default="src",
                      advanced=True),
            )),
            Group("Budgets (optional)", (
                Field("resources.ram_budget_bytes", ("resources", "ram_budget_bytes"), "RAM budget (bytes)",
                      "Shown in the review for comparison.", kind="optional_int", default=None,
                      minimum=1, maximum=2147483647),
                Field("resources.transport_bytes_per_second", ("resources", "transport_bytes_per_second"),
                      "Link budget (bytes/s)", "Used to estimate the link load in the review, "
                      "e.g. 11520 for 115200 baud.", kind="optional_int", default=None,
                      minimum=1, maximum=2147483647),
            ), advanced=True),
        ),
        guided_title="Where should the configuration go?",
        guided_intro="The generator writes a snapshot that your build includes. "
                     "Nothing in your own source files is overwritten.",
        diagnostic_prefixes=("build", "resources"),
        summary=_summary_build, order=60),
    Section(
        "review", "Review & generate",
        "Effective settings, open problems and the files that will be generated.",
        widgets=("review",),
        guided_title="Review and generate",
        guided_intro="Check the summary. 'Generate' writes the configuration files that your firmware "
                     "build includes. You can come back and generate again at any time.",
        order=1000),
]


def sections() -> list[Section]:
    """All registered sections in display order."""
    return sorted(SECTIONS, key=lambda s: s.order)


def section(key: str) -> Section:
    return next(s for s in SECTIONS if s.key == key)


def register_section(new: Section) -> None:
    """Add or replace a section; it appears in both GUI modes."""
    unregister_section(new.key)
    SECTIONS.append(new)


def unregister_section(key: str) -> None:
    SECTIONS[:] = [s for s in SECTIONS if s.key != key]


def section_for_path(path: str, profile: str) -> Section | None:
    """Section that owns a diagnostic path, longest prefix first."""
    best = None
    best_length = -1
    for candidate in SECTIONS:
        for prefix in candidate.diagnostic_prefixes:
            concrete = prefix.format(profile=profile)
            if candidate.owns(path, profile) and len(concrete) > best_length and \
                    (path == concrete or path.startswith(concrete)):
                best, best_length = candidate, len(concrete)
    return best
