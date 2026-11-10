# Changelog

## Unreleased (7.3.0)

- Shared transport: move the integration example's frame sender into the optional
  C-compatible `KymoTx` module (`KYMO_ENABLE_TX=1`, off by default). Caller-owned
  contexts and buffers support independent transports, partial writes, backpressure
  and borrowed-buffer completion without heap allocation. Init refuses to reset an
  existing sender; driver contract violations latch a fault. The example now uses
  this library module; module scheduling remains in the application, without an Endpoint.

- Documentation: move protocol, contribution rules and publishing instructions into
  `docs/`; add a documentation index, remove the completed 7.0 structure review
  and redundant 3D note, and correct outdated build/setup instructions.

- Live variables: `control/kymo_watch.h` lets the host sample global or static
  variables that it picked from the firmware's ELF file (`KYMO_ENABLE_WATCH`, needs
  `KYMO_ENABLE_DECODER`; `KYMO_MAX_WATCHES`, default 8; `KYMO_WATCH_MIN_PERIOD_MS`,
  default 5). Reading only. The firmware declares the readable memory ranges and a
  build ID; every WATCH_SET is checked (slot, type, alignment, range, period) and
  answered with WATCH_ACK. `KymoWatch_NextSample` reads one due variable per call with
  exactly its width and returns an ordinary DATA point; an optional reader callback
  locks 8-byte reads or maps addresses on host builds. Protocol: operations
  0x03..0x06 on the control descriptor `0x60`, see [docs/PROTOCOL.md](docs/PROTOCOL.md).
- Back channel: KymoParam and KymoWatch share the frame reassembly
  (`control/kymo_control_rx.h`) and may be fed the same bytes; each ignores the
  other's control operations. Host requests may now carry up to 16 payload bytes
  (was 8); unknown operations still count as invalid.
- Configurator: feature "Live variables from the ELF file" (`watch`). The optional
  top-level `watch` object holds the readable ranges as linker symbols (presets for
  ESP32, STM32CubeIDE and CMSIS GCC/RP2040/nRF, a second range for external RAM), a
  build ID callback, the slot count and the shortest period. The generator writes
  `kymo_generated_watch.cpp` with `kymo_generated_watch_init()`, so the firmware no
  longer fills `KymoWatchConfig` by hand. Older projects stay valid.
- GUIDE: setting up live variables, linker symbols per toolchain and the GNU build ID.
- CRC tables: `KYMO_ENABLE_CRC_TABLE` (default 0) computes the CRC-8 of data and control
  frames and the trace CRC-16 by table lookup instead of bit by bit. Results are
  unchanged; flash grows by 244 bytes (CRC-8) and 486 bytes (CRC-16) on Cortex-M4.
  Worth it on fast transports. Native tests compare both against the bitwise
  reference. Configurator: feature "CRC lookup tables" (`crc_table`).
- Configurator: RAM estimate of the KymoCore state objects per profile (from sizes
  measured on Cortex-M4); the review warns when it exceeds `resources.ram_budget_bytes`.
- Memory telemetry: `telemetry/kymo_memory.h` (`KYMO_ENABLE_MEMORY`, no decoder needed;
  `KYMO_MAX_MEMORY_ENTRIES`, default 8) reports task stack peaks and heap levels as named
  curves. The firmware lists the entries and reads the values in a callback; KymoCore
  calls no RTOS function. MEMORY_INFO (operation 0x07 on the control descriptor `0x60`)
  describes each entry with channel, kind, total size and name, unsolicited on start,
  periodically and on request. About 0.6 KB flash on Cortex-M4. Configurator: feature
  "Memory telemetry" (`memory`).
- Trace: `KYMO_ENABLE_TRACE_STACK_POINTER` (default 0, needs `KYMO_ENABLE_TRACE`) puts
  the stack address into the otherwise unused `arg` of ENTER and EXIT and sets the new
  flag `KYMO_TRACE_FLAG_STACK_SAMPLE` (0x0010). Events stay 32 bytes. The address comes
  from `__builtin_frame_address(0)`; other compilers define
  `KYMO_TRACE_READ_STACK_POINTER()`. Configurator: trace option "Stack pointer".

## 7.2.1 - 2026-11-03

- Links: repository, documentation and issue URLs in `library.json`,
  `library.properties`, `idf_component.yml` and the docs point to the KymoTrace
  organization (github.com/KymoTrace/kymocore). No change to code, API or protocol.

## 7.2.0 - 2026-10-29

- License: KymoCore is now MIT-licensed (`SPDX-License-Identifier: MIT`) instead of
  GPLv3 or a commercial license; COMMERCIAL.md is gone. Earlier releases keep the
  license they were published with. No change to code, API or protocol.

## 7.1.0 - 2026-10-25

- Back channel: `control/kymo_param.h` lets the host write registered float
  parameters with ordinary CRC-protected DATA frames (`KYMO_ENABLE_PARAMETERS`,
  needs `KYMO_ENABLE_DECODER`; `KYMO_MAX_PARAMETERS`, default 8). Writes outside
  [minimum, maximum], unknown IDs, NO_CRC and bad frames are refused. Accepted and
  out-of-range writes queue a readback that the application sends as normal DATA.
  Byte-wise reassembly with constant work per byte, no heap, compiled out by default.
- Parameter discovery: `KymoParameter` carries `name` and `unit`; `KYMO_PARAM(id, var,
  min, max, unit)` names the entry after its variable. A host list request
  (descriptor `0x60`, operation LIST) queues one description per parameter, sent with
  `KymoParam_NextDescription`, plus a readback of every current value.
- `emb_bitset_clear` in `common/embedded_bits.h`.
- `KymoParam_FeedCompact` applies a 5-byte write (ID, float32 LE) received over a link with
  its own framing such as CAN; same rules as `KymoParam_Feed`, no discovery.
- Configurator: optional feature `parameters` (`KYMO_ENABLE_PARAMETERS`), checked to need the
  frame decoder. Projects without the key stay valid and keep it off; schema 2 is unchanged.
- ESP-IDF component manifest (`idf_component.yml`, root `CMakeLists.txt`) for the
  ESP Component Registry and a plain ESP-IDF example, `examples/esp_idf/uart_telemetry`.
- CI lints the library for the Arduino Library Manager and builds the ESP-IDF example;
  release tags can upload to the ESP Component Registry once it is set up (PUBLISHING.md).

## 7.0.0 - 2026-10-18 (not tagged; published together with 7.1.0)

Restructured for a simpler start. The wire format (protocol v6.2, wire version 1)
and the feature set are unchanged; the API and file layout change. See
"Migrating from KymoCore 6.x" in docs/GUIDE.md for the complete mapping.

- One entry header `KymoCore.h`; sources grouped into `telemetry/`, `trace/`,
  `transport/` and `common/`. Arduino `includes=` and PlatformIO `headers` point
  to `KymoCore.h`.
- One configuration header `kymo_config.h` (replaces `kymo_build_config.h`,
  `kymo_features.h`, `kymo_version.h`) with version macros, all switches and all
  resource sizes. New `KYMO_PROFILE` presets: `KYMO_PROFILE_MINIMAL` (default),
  `KYMO_PROFILE_XYZ`, `KYMO_PROFILE_TRACE`. `KYMO_CONFIG_API_VERSION` is 2.
- `kymo_types.h`: one `KymoStatus` for every module (new `KYMO_REJECTED`) and
  shared `KymoClockFn`, `KymoWriteFn`, `KymoBusyFn`, `KymoServiceFn`, `KYMO_COUNT_OF`.
- Runtime: `Kymo_Init` copies `KymoConfig` (it may be local); required fields
  come first; `last_sample_ms` is removed and the schedule lives in
  `KymoContext`, sized by `KYMO_MAX_CHANNELS` (default 16, up to 256).
- Codec: `KymoProtocol_Encode`, `KymoProtocol_Decode` (returns `KymoStatus`),
  `KymoProtocol_FrameLength`, `KymoProtocol_Crc8`.
- C++: `Kymo` becomes `KymoSender` in `telemetry/kymo_sender.h` together with
  `KymoStream`; `send*`/`flush` return `KymoStatus`; `setClock(KymoClockFn, user)`
  replaces `setMillisecondCallback`.
- Trace: `KymoTrace_Init`, `KymoTrace_Pop` and `KymoTrace_ContextReset` return
  `KymoStatus`; `core_id` and `current_context` callbacks are optional.
- Mux: `KymoTraceMux*` becomes `KymoMux*` in `transport/kymo_mux.h`; config fields
  `write`, optional `busy`, `transport_user` and new `data_user`; zero
  `max_frame_size`/`max_batch_events` select the maximum; `KymoTraceDataAdapter`
  becomes `KymoMuxDataSlot`; `KYMO_TRACE_ADMIN_RECORD_MAX` becomes
  `KYMO_MUX_ADMIN_RECORD_MAX`; `KymoMux_Service` returns `KYMO_OK`, `KYMO_IDLE`,
  `KYMO_BUSY` or `KYMO_IO_ERROR`.
- `common/`: CRC-16/CCITT, CRC-32 and u16/u64 writers shared by trace and mux.
- Self-contained Arduino examples without a shared header; named-field setup.
- New `test/native_test.py` (runtime, sender, common utilities, profiles,
  structure check); CI runs it.
- Configurator 0.2: project schema 2 with migration from schema 1, KymoCore 7.0
  code generation (sizes `KYMO_MAX_CHANNELS`, optional trace callbacks), use-case
  presets matching `KYMO_PROFILE`, a guided step-by-step mode and a professional
  mode built from one declarative setting catalog with explanations and affected
  macros, and extension hooks for future features (see `tools/configurator/EXTENDING.md`).

## 6.4.0 - 2026-10-15

- Add `KYMO_FLAG_TIMESTAMP_US` (descriptor bit 4, data message type 01): the timestamp
  field carries uint32 microseconds instead of milliseconds. Frame layout and length are
  unchanged. Valid only together with `KYMO_FLAG_TIMESTAMP`; available with
  `KYMO_ENABLE_TIMESTAMP=1` through `kymo_encode_data`/`kymo_decode_data` (`KYMO_CODEC_FLAGS`).
- The channel runtimes keep millisecond clocks and reject the µs flag; `KYMO_ALLOWED_FLAGS`
  and `KYMO_SUPPORTED_FLAGS` are unchanged.
- `KYMO_DESCRIPTOR_RESERVED_MASK` is now `0x20`. Older receivers reject µs frames.

## 6.3.2 - 2026-10-12

- Add small Arduino reference examples (multi sensor, simple analog) and an examples overview.
- Set the package homepage to https://kymotrace.de.
- Exclude `.pio` and `.vscode` folders from the exported package.
- Library code, API and wire behavior are unchanged.

## 6.3.1 - 2026-10-11

- Improve readability of the configuration, instrumentation, recorder, protocol and mux headers.
- Separate declarations and enum entries, wrap long API declarations and add spacing between definitions.
- Formatting-only library changes; API, resource layout and wire behavior are unchanged.


## 6.3.0 — 2026-10-11


- Add global compile-out and optional trace/ISR/scheduler/flow feature gates; disabled instrumentation does not evaluate arguments.
- Add a bounded, allocation-free trace recorder and CRC-protected wire-v2 protocol, preserving legacy DATA v1.
- Add a fair transport multiplexer with partial-write, backpressure and borrowed-buffer ownership handling.
- Add the CLI and optional Qt configurator, immutable hashed generations, GCC/CMake/PlatformIO exports and generated trace/runtime wiring.
- Include shared protocol fixtures and executable generated-code regression tests.
- Trace features default off. ISR and scheduler hardware coverage depends on an independently validated application port; no hardware timing or ISR-safety certification is claimed.
- Trace visualization requires a KymoStudio build with wire-v2 trace support. Legacy scalar/XYZ telemetry remains available.

## 6.2.3

- Prepare the PlatformIO Registry release under `codename666/KymoCore`.
- Replace installation placeholders with the confirmed registry namespace.
- No change to code, API or protocol.

## 6.2.2

- Documentation is now English by default with a German version in parallel:
  `README.md`/`README.de.md`, `docs/GUIDE.md`/`docs/GUIDE.de.md`,
  `COMMERCIAL.md`/`COMMERCIAL.de.md`, `CONTRIBUTING.md`/`CONTRIBUTING.de.md`.
  A language switch at the top of each file toggles between them.
- `docs/ANLEITUNG.md` was renamed to `docs/GUIDE.de.md`.
- No change to code, API or protocol.

## 6.2.1

- README als Übersicht mit Logo, Screenshot, Einsatzgebieten und Schnellstart.
- Ausführliche Anleitung und API-Referenz in `docs/ANLEITUNG.md` (im Paket enthalten).
- Diagramme mit den Namen KymoCore/KymoStudio neu erzeugt.
- Keine Änderung an Code, API oder Protokoll.

## 6.2.0

- **Lizenzwechsel:** KymoCore ist ab dieser Version doppelt lizenziert:
  GPLv3 oder kommerzielle Lizenz (`MIT`).
  Siehe COMMERCIAL.md. Versionen bis einschließlich 6.1.0 bleiben MIT-lizenziert.
- Alle Quelldateien tragen eine SPDX-Kennung und den Copyright-Vermerk.
- CONTRIBUTING.md beschreibt die Rechteeinräumung für Beiträge.
- Keine Änderung an API, Protokoll oder Wire-Format gegenüber 6.1.0.
