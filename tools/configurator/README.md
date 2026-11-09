# KymoCore Configurator

Creates the build configuration for **KymoCore 7** (configuration API 2): which
features are compiled in, which channels are sent and which application functions
connect KymoCore to the hardware. The configurator is an optional development tool. Its Python engine and CLI use only the standard library; PySide6 is needed only for the Widgets editor. Firmware builds consume fixed generated C/C++ snapshot files and do not launch Python or Qt.

## Install and start

Use Python 3.12. From the KymoCore repository:

```powershell
python -m venv .venv-configurator
.venv-configurator\Scripts\python -m pip install -e .\tools\configurator
.venv-configurator\Scripts\python -m kymo_configurator new kymo.project.json --name KymoProbe
.venv-configurator\Scripts\python -m kymo_configurator gui kymo.project.json --mode guided
```

## Two ways to work

- **Guided setup** (`gui --mode guided`): asks what you want to achieve (values over time are
  always included; X/Y or X/Y/Z points and execution tracing can be ticked on top), then walks through channels,
  callbacks, optional tracing and output step by step. Advanced options keep safe
  defaults; a step with open problems cannot be left forward.
- **Professional mode** (`gui --mode professional`): every setting at once, grouped
  by topic, with live status per topic (✓/⚠/✖), inline problems, a review page with
  the effective macros and a preview of the generated files.

Without `--mode` a start page offers both. The interface is available in English and German: `--lang en|de` or the language selector in the toolbar (remembered); without a choice it follows the system language. The GUI also shows diagnostics in the selected language; the CLI and `--json` output stay English. Both modes edit the same project, so you
can switch at any time. Every setting shows a plain-language explanation and the C
macro, configuration field or function signature it affects.

Install the optional GUI with `pip install -e "tools/configurator[gui]"`. The Python package intentionally does not pin a different Qt runtime than the host application. The current supported development range is PySide6 `>=6.8.3,<6.10`; validation in this workspace used Python 3.12.9 and PySide6 6.9.3.

## CLI

```text
python -m kymo_configurator validate kymo.project.json --profile development
python -m kymo_configurator generate kymo.project.json --profile development --out generated/kymo/development
python -m kymo_configurator check kymo.project.json --profile development --out generated/kymo/development
python -m kymo_configurator.build_check compile_commands.json
python -m kymo_configurator migrate old.project.json --output kymo.project.json
python -m kymo_configurator init-port kymo.project.json --out src/kymo_port
python -m kymo_configurator gui kymo.project.json --mode guided
```

Diagnostics have stable codes and JSON field paths; `validate --json` emits them as JSON. Validation and `check` do not write files. Migration upgrades schema 1 (KymoCore 6.x) projects to schema 2 (KymoCore 7.0) and writes a new copy unless `--output` is specified; the GUI upgrades older projects on open. Generation creates an immutable `generations/<sha256>/` directory and atomically switches `active_generation.json`; `--force` is required to replace a manually edited currently selected generated file. The fixed snapshot can be pinned in a build so a concurrent generation cannot mix headers and sources.

`build_check` reads the compiler's `compile_commands.json` argument arrays and verifies the effective feature macros and the actual included generated configuration for each listed translation unit. It does not start or modify the build.

`init-port` creates a callback header/source scaffold in a new directory and refuses to overwrite an existing path. The source deliberately fails compilation while runtime is enabled until target behavior replaces its TODO stubs. With an active trace profile it also creates the trace clock, session, write and busy stubs. It does not invent clock, sensor, or transport behavior.

## Generated files and build exports

Each snapshot contains `kymo_user_config.h`, `kymo_generated_config.h/.cpp`, trace ID/dictionary development files, a manifest with hashes, and GCC, CMake, and PlatformIO export files. PlatformIO uses both `kymo_build_export.py` (pre-build global include/definitions) and `kymo_build_sources.py` (generated source). The C++ configuration follows the KymoCore 7 API: a static `KymoChannel` table and a `KymoConfig` with `channels`, `channel_count`, `clock_ms`, `sample`, `write`, optional `busy`/`service` and user pointer bindings; `KYMO_MAX_CHANNELS` is sized to the channel table. It references callbacks by validated identifier and never owns their implementations. Keep those implementations in a hand-maintained port source file.

When the optional C++ sender is enabled, `bindings.cpp_stream` names a user-owned `KymoStream &` reference, which may refer to an instance of an application subclass. Do not declare an abstract `KymoStream` object. The generated `kymo_generated_sender` is default-constructed without calling user bindings; after the stream and clock are ready, call `kymo_generated_init_cpp_sender()` before using the sender. With effective timestamps enabled, initialization installs the configured no-argument `uint32_t` clock through `KymoSender::setClock` and samples it once as the start epoch. With timestamps disabled, an omitted `cpp_timestamp_default` defaults to false. An explicit true request while effective timestamps are off is invalid.

The generated user header is intended to be selected project-wide using `KYMO_USER_CONFIG_HEADER`, so application and Core translation units resolve the same values. For CMake, include `kymo_build_export.cmake` and call `kymo_configure_targets(application_target core_target)` after defining both targets. GCC variables can be included in a make-based build; PlatformIO uses the generated pre/post scripts to add the snapshot include path and generated `.cpp`. Run the resulting firmware build to verify the specific integration.

An enabled trace profile emits canonical `kymo_trace_metadata.json` and `kymo_trace_dictionary.json` bodies plus a C-compatible `kymo_trace_metadata.h`. Its byte arrays can be passed to `KymoMux_QueueAdmin`; that API appends the CRC32 trailer. The dictionary SHA-256 is included in both metadata and the header. Buffer sizes and core count become compile-time settings in `kymo_user_config.h`, and the admin staging bound is sized for the generated records (up to Core's 8192-byte limit). Scheduler/ISR/core-state completeness stays false unless the project explicitly sets the corresponding `trace.port.*_complete` claim and marks its top-level port capabilities as verified. Port names do not create capability or RAM claims.

## Extending

Additional KymoCore features are added as one catalog
section plus optional widget, validator and renderer; both GUI modes pick them up
automatically. See [EXTENDING.md](EXTENDING.md).

See [README.de.md](README.de.md) for German instructions and `schema/kymo-project-v2.schema.json` for the JSON structure.
