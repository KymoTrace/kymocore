# KymoCore – Guide and reference

[← Back to overview](../README.md) · **English** · [Deutsch](GUIDE.de.md)

This guide explains setup, data path and protocol. The technical API reference
at the end describes the detailed contracts.

## Architecture and data path

![From measurement source to KymoStudio](images/architecture.png)

The application owns the measurement sources, clock and transport drivers.
`Kymo_Init` validates the configuration and copies it into a context.
`Kymo_Main` selects a due channel, calls the measurement callback, encodes the
data point and offers the bytes to the transport. KymoStudio reassembles
received fragments and assigns measurements by their channel ID.

| Layer | Task | Interface |
|---|---|---|
| Application | Sensors, hardware initialisation and connection | Your own drivers |
| Time source | Provide monotonic milliseconds | `clock_ms(user)` |
| Measurement source | Read Y and, if needed, X/Z | `sample(user, id, out)` |
| Scheduler | Due times, round robin and backpressure | `Kymo_Init`, `Kymo_Main` |
| Codec | Check values and build a little-endian frame | `KymoProtocol_Encode` |
| Transport adapter | Copy bytes or borrow them in a controlled way | `write`, optional `busy`/`service` |
| KymoStudio | Decode, display and analyse signals | [Desktop project](https://kymotrace.de/#download) |

The optional C++ class `KymoSender` is an alternative push sender: the
application decides when to send. Both APIs use the same wire format. There is
no measurement queue in the core.

## Library layout

Applications include **one header**, `<KymoCore.h>`. It pulls in every module
the build configuration enables. The sources are grouped by task:

| Folder / header | Module | Enabled by |
|---|---|---|
| `kymo_config.h` | Version, profile, all switches and sizes | always |
| `kymo_types.h` | `KymoStatus` and the shared callback types | always |
| `telemetry/kymo_protocol.h` | DATA frame codec (`KymoProtocol_*`) | always |
| `telemetry/kymo_runtime.h` | `Kymo_Init` / `Kymo_Main` | `KYMO_ENABLE_RUNTIME=1` (default) |
| `telemetry/kymo_sender.h` | `KymoSender` + `KymoStream` (C++) | `KYMO_ENABLE_CPP=1` |
| `trace/kymo_instrumentation.h` | `KYMO_TRACE_*` macros | always (compile out) |
| `trace/kymo_trace.h` | `KymoTraceRecorder` | `KYMO_ENABLE_TRACE=1` |
| `transport/kymo_mux.h` | `KymoMux`: trace + DATA on one link | `KYMO_ENABLE_TRACE=1` |
| `common/` | Bit, byte and CRC helpers | always |

Every function that can fail returns `KymoStatus`, where `KYMO_OK` (0) always
means success. All modules share `KymoClockFn`, `KymoWriteFn`, `KymoBusyFn`
and `KymoServiceFn`, so one transport adapter serves runtime, sender and mux.

**The whole package is platform-independent.** No file includes an SDK,
vendor or framework header (no `Arduino.h`, HAL or RTOS headers), and no
target macro changes the API. Hardware access always lives in application
callbacks; the Arduino sketches under `examples/` are such applications.

## Installing with PlatformIO

Install KymoCore from the PlatformIO Registry using the `codename666` namespace:

```ini
[env:esp32dev]
platform = espressif32@6.12.0
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    codename666/KymoCore @ 7.2.0
```

For Uno, set `platform = atmelavr@5.3.0` and `board = uno` instead. The library
itself is tied neither to these boards nor to Arduino.

Until it is published in the registry, install directly from GitHub. The tag
pins the version:

```ini
lib_deps =
    https://github.com/KymoTrace/kymocore.git#v7.2.0
```

Alternatively, build a package from this repository:

```sh
pio pkg pack . -o KymoCore-7.2.0.tar.gz
```

Copy the archive into your project and reference it there:

```ini
lib_deps =
    file://KymoCore-7.2.0.tar.gz
```

Alternatively, copy the complete library folder to `lib/KymoCore` of your own
project.

## Complete Arduino and ESP32 example

The following content for `src/main.cpp` sends a synthetic sawtooth between 0
and just below 1 on channel 0 every 20 ms. It uses only the default settings.
The serial interface is reserved for binary data; diagnostic output would
corrupt the same data stream.

```cpp
#include <Arduino.h>
#include <KymoCore.h>

static KymoContext context = {};
static KymoStatus lastStatus = KYMO_BAD_CONFIG;
/* period_ms, id, flags: channel 0, every 20 ms, scalar value only. */
static const KymoChannel channels[] = {{20U, 0U, 0U}};

/*******************************************************************************
 * readClockMs
 ******************************************************************************/
static uint32_t readClockMs(void *user)
{
    uint32_t result = static_cast<uint32_t>(millis());
    (void)user;
    return result;
}

/*******************************************************************************
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *user, uint8_t id, KymoSample *out)
{
    uint8_t result = 0U;
    (void)user;
    (void)id;
    if (out != nullptr) {
        out->value = static_cast<float>(millis() % 1000UL) / 1000.0f;
        result = 1U;
    }
    return result;
}

/*******************************************************************************
 * writeBytes
 ******************************************************************************/
static uint8_t writeBytes(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t result = 0U;
    int available = Serial.availableForWrite();
    (void)user;
    if ((bytes != nullptr) && (available > 0)) {
        if (available < length) {
            length = static_cast<uint8_t>(available);
        }
        result = static_cast<uint8_t>(Serial.write(bytes, length));
    }
    return result;
}

/*******************************************************************************
 * setup
 ******************************************************************************/
void setup()
{
    KymoConfig config = {};   /* copied by Kymo_Init, may be local */
    Serial.begin(115200);
    config.channels = channels;                     /* what */
    config.channel_count = KYMO_COUNT_OF(channels);
    config.clock_ms = readClockMs;                  /* when */
    config.sample = readSample;                     /* which values */
    config.write = writeBytes;                      /* where to */
    lastStatus = Kymo_Init(&context, &config);
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop()
{
    if ((lastStatus != KYMO_BAD_CONFIG) && (lastStatus != KYMO_IO_ERROR)) {
        lastStatus = Kymo_Main(&context);
    }
}
```

`int available` follows the return type of the Arduino API; the value range is
checked before narrowing. Serial copies the accepted bytes, so no busy callback
is needed here. The actual execution time depends on the Serial driver. Later,
replace the measurement callback with your sensor access.

Build with `pio run`, then flash the connected board with `pio run -t upload`.
Connect KymoStudio at 115200 baud and 8N1 and assign channel 0 to a chart.
Close any open serial monitor first. A completely SDK-free, executable example
is in [examples/basic](../examples/basic/README.md).

## Features and configuration

All settings live in **`src/kymo_config.h`**. Precedence: compiler definitions
(`-D...`) override an optional project header named by
`KYMO_USER_CONFIG_HEADER`, which overrides the profile, which overrides the
library defaults. For installed registry packages, project-wide `build_flags`
are the reproducible choice.

**Profiles** set sensible defaults in one step; each switch can still be set
individually:

| Profile | Enables |
|---|---|
| `KYMO_PROFILE_MINIMAL` (default) | Scalar Y values, Init/Main runtime |
| `KYMO_PROFILE_XYZ` | X, Z and timestamps |
| `KYMO_PROFILE_TRACE` | XYZ plus trace recorder with ISR, scheduler and flow events |

```ini
build_flags =
    -DKYMO_PROFILE=KYMO_PROFILE_XYZ
    -DKYMO_ENABLE_CRC=1
```

| Switch | Default | Effect when 1 |
|---|---:|---|
| `KYMO_ENABLED` | 1 | 0 compiles the whole library out |
| `KYMO_ENABLE_RUNTIME` | 1 | Init/Main scheduler |
| `KYMO_ENABLE_CPP` | 0 | C++ push sender `KymoSender` |
| `KYMO_ENABLE_X` | profile | Optional X coordinate |
| `KYMO_ENABLE_Z` | profile | Optional Z coordinate, independent of X |
| `KYMO_ENABLE_TIMESTAMP` | profile | Timestamp on the wire |
| `KYMO_ENABLE_CRC` | 0 | Compute outgoing CRC |
| `KYMO_ENABLE_DECODER` | 0 | Decode complete frames on the MCU |
| `KYMO_ENABLE_PARAMETERS` | 0 | Host writes to registered parameters, `KymoParam_*` (needs DECODER) |
| `KYMO_ENABLE_TRACE` | profile | Trace recorder, instrumentation and mux |
| `KYMO_ENABLE_TRACE_ISR` | profile | ISR events (needs TRACE) |
| `KYMO_ENABLE_TRACE_SCHEDULER` | profile | Task events (needs TRACE_ISR) |
| `KYMO_ENABLE_TRACE_FLOW` | profile | Flow correlation (needs TRACE) |

| Size | Default | Range |
|---|---:|---|
| `KYMO_MAX_CHANNELS` | 16 | 1–256 channels per context, 4 bytes each |
| `KYMO_TRACE_EVENTS_PER_CORE` | 64 | Power of two, 16–4096 |
| `KYMO_TRACE_CORE_COUNT` | 1 | 1–2 |
| `KYMO_TRACE_MAX_CONTEXTS` | 16 | 1–256 |
| `KYMO_TRACE_MAX_DEPTH` | 32 | 1–255 |
| `KYMO_TRACE_MAX_ISR_DEPTH` | 8 | 1–16 |
| `KYMO_MUX_ADMIN_RECORD_MAX` | 512 | 64–8192 bytes |
| `KYMO_MAX_PARAMETERS` | 8 | 1–256 parameters per `KymoParamReceiver`, 1 bit each |

Enabling a feature allows a field; each channel's flags select it for that
channel's frame. XYZ with timestamp uses `KYMO_ALLOWED_FLAGS`, timestamp only
`KYMO_FLAG_TIMESTAMP`. The callback sets X/Z; the runtime computes the time
since Init. Rebuild all units together: a define in the sketch alone does not
configure separately compiled library files.

Only 0 and 1 are allowed; invalid values and unmet dependencies stop the build.
Disabled implementations are compiled out. Invalid field requests are
rejected, not silently dropped. The MCU decoder checks protected input even
when outgoing CRC is disabled. The desktop decoder is independent of firmware
features. Test `KYMO_EFFECTIVE_*` in application code to guard optional
resources.

## Scheduling and transmission flow

![Main calls, partial transfer and asynchronous buffer ownership](images/transmission.png)

1–256 unique channel IDs are possible. Periods range from 0 to `INT32_MAX` ms;
0 means on every possible selection. After Init all channels are due
immediately. Main checks at most N channels and calls the measurement callback
at most once and Write at most once. Round robin prevents permanent
starvation. Call Main more often than the sum of the desired sampling rates;
short writes need further calls.

Each context has **one 21-byte TX buffer**, plus further context fields and
**4 bytes of scheduling state per channel**. The whole context is larger than
21 bytes and ABI-dependent. Configuration, stack and driver buffers come on
top. A pending frame keeps its measurement. Afterwards current values are
requested; missed samples are not reconstructed.

`write(user, bytes, length)` reports the number of accepted bytes. Zero means
try again later; a short write advances the offset. The next Main call
continues with the rest. A return value greater than the requested length
latches `KYMO_IO_ERROR` until a safe re-initialisation.

With synchronous transmission, the callback must copy or consume accepted bytes
before returning. With DMA/USB the driver may keep the pointer if `busy(user)`
stays non-zero until release. Meanwhile the buffer is not overwritten.
`KYMO_OK` means acceptance by the driver, not physical completion or a receipt.
`service(user)` runs at the start of every valid Main call, even when busy and
with a latched error.

Kymo_Init copies the configuration; only the channel table and user contexts
must outlive the context. Each instance owns its own context, which also holds
the per-channel schedule. The APIs are not reentrant; the application takes
care of task/ISR synchronisation. Do not re-initialise or destroy while a
driver borrows the buffer. During partial frames, other producers must not mix
bytes into the same stream.

## Protocol and byte layout

![Frame fields, descriptor bits and concrete wire examples](images/protocol.png)

Package version 7.2.0, protocol v6.2 and **wire version 1** are different
things. The normative contract is in [PROTOCOL.md](../PROTOCOL.md).

| Offset | Size | Content |
|---|---:|---|
| 0 | 1 byte | Sync `A5` |
| 1 | 1 byte | Sync `5A` |
| 2 | 1 byte | Descriptor |
| 3 | 1 byte | Channel ID 0–255 |
| 4 | 0/4 bytes | Optional X |
| 4 or 8 | 4 bytes | Y, always present |
| after Y | 0/4 bytes | Optional Z |
| after Z or Y | 0/4 bytes | Optional time in ms or µs |
| last byte | 1 byte | CRC-8/ATM or zero trailer |

The payload order is **X? → Y → Z? → time?**. X/Y/Z are finite IEEE-754
float32 values; NaN and infinity are rejected. Time is an unsigned 32-bit
millisecond value relative to sender start and wraps after about 49.7 days.
With the microsecond bit it counts microseconds and wraps after about 71.6 minutes.
Multi-byte values are little endian: least significant byte first. No packed
C structures are transmitted directly.

| Descriptor bits | Meaning |
|---|---|
| 7–6 | `01` = wire version 1 |
| 5 | `0` = data point |
| 4 | Time unit: 0 = ms, 1 = µs (only with bit 1) |
| 3 | X present |
| 2 | Z present |
| 1 | Timestamp present |
| 0 | `NO_CRC`: 1 without CRC, 0 with CRC |

`0x40` to `0x4F` are valid: even values with CRC, odd values without CRC.
`0x52`/`0x53`, `0x56`/`0x57`, `0x5A`/`0x5B` and `0x5E`/`0x5F` are the same layouts
with microsecond time; `0x50`, `0x54`, `0x58` and `0x5C` (µs without time and
their NO_CRC variants) are invalid. Set `KYMO_FLAG_TIMESTAMP | KYMO_FLAG_TIMESTAMP_US`
on a `KymoDataPoint` for `KymoProtocol_Encode`. The channel runtimes use
millisecond clocks and reject the µs flag. Receivers older than KymoCore 6.4 /
KymoStudio 2.5 reject µs frames as an unsupported message type.
Other versions and message types are rejected. Firmware features can restrict
the supported layouts further.

**Length = 9 + 4 × (X present + Z present + time present)**, where each term is
0 or 1. A separate length byte is not needed.

| Layout | Without time | With time |
|---|---:|---:|
| Y | 9 bytes | 13 bytes |
| XY | 13 bytes | 17 bytes |
| YZ | 13 bytes | 17 bytes |
| XYZ | 17 bytes | 21 bytes |

### Concrete frames

Channel 7, Y = 1.0, without time and without CRC:

```text
A5 5A | 41 | 07 | 00 00 80 3F | 00
Sync    Desc ID   Y = 1.0        Trailer
```

`0x41 = 01000001`: version 1, data point, no optional fields, NO_CRC set.
`00 00 80 3F` is little endian for float32 `0x3F800000`. With CRC the frame is
`A5 5A 40 07 00 00 80 3F 54`.

Channel 3, X = 1.25, Y = −2.5, Z = 9.0, time = 1234 ms:

```text
A5 5A | 4F | 03 | 00 00 A0 3F | 00 00 20 C0 | 00 00 10 41 | D2 04 00 00 | 00
Sync    Desc ID   X = 1.25      Y = -2.5      Z = 9.0       time = 1234   Trailer
```

The full frame has 21 bytes. With CRC, `4F` becomes `4E` and the last byte
becomes `89`. The app's Python data points represent the wire milliseconds as
seconds: 1234 ms correspond to 1.234 s.

### CRC and resynchronisation

CRC is disabled by default. The sender sets NO_CRC and writes a zero trailer
byte. The receiver ignores its value. Sync, descriptor, length and finiteness
checks stay active but do not detect every corruption: a corrupted finite
measurement can get through.

With `KYMO_ENABLE_CRC=1`, CRC-8/ATM applies: polynomial `0x07`, initial value
`0x00`, no reflection, XOR-out `0x00`. **Descriptor, ID and payload** are
protected; sync and the CRC byte itself are excluded. ASCII `123456789` yields
`F4`. After invalid frames, the desktop stream decoder searches for `A5 5A`
again. Transport read boundaries are not frame boundaries.

Older v6.0 receivers do not accept NO_CRC. Therefore use an updated app or
enable CRC. The current desktop decoder handles both modes, even mixed. The
optional MCU decoder expects a complete frame; it is not a stream assembler.

## Transports and sizing

| Transport | Connection by the application | Framing |
|---|---|---|
| UART / Serial | Copying write or DMA plus busy | Partial frames possible |
| USB CDC | Copy or borrow the buffer plus busy | USB packets are not frame boundaries |
| TCP | Socket write, optional service | Continuous byte stream |
| UDP | Socket `sendto` in the write callback | One whole frame per datagram or report 0 |
| MQTT | Network service and publish callback | Accept the whole frame or report 0 |
| CAN FD | Embed the whole frame in the data field | Remove padding before decoding |
| Classic CAN | Own compact mapping | No 9–21 byte frame in an 8-byte packet |

Classic CAN uses a channel mapping via arbitration ID or adapter configuration
and a configured number format. Details are in the
[protocol contract](../PROTOCOL.md#can). Network setup and reconnection belong
to the adapter. The external PubSubClient example connects synchronously and is
therefore no proof of a hard real-time cycle.

UART at 115200 baud and 8N1 needs ten line bits per byte. In theory 11,520
bytes/s are possible: about 1,280 Y data points/s or 548 XYZ data points/s with
timestamp, each without further gaps or driver overhead. Ten channels at 100 Hz
each need 9,000 bytes/s as plain Y frames; with timestamp 13,000 bytes/s, more
than this UART can carry. Size the measurement rate, Main call frequency and
transport together.

## Connecting KymoStudio

The matching desktop application is
**[KymoStudio](https://kymotrace.de/#download)**; the Windows installer is
available on the download page. The library
delivers telemetry; the app handles reception and visualisation.

1. Use an app decoder with v6.1/NO_CRC support or enable firmware CRC. Older
   app versions may lack this support.
2. Flash the firmware and configure the input in the app, for example Serial at
   115200 baud/8N1, UDP on the port the firmware sends to, or MQTT with the
   publisher's topic.
3. Assign the channel ID to a signal/chart. Y is the measurement; X/Z are
   optional coordinates. Without X, no measured X value is transmitted.
4. First check the known example waveform, then connect sensors.

The protocol does not transmit channel names, units or sensor configurations.
This mapping stays with the application and the app. Since 7.1 the optional
parameter back channel (`KYMO_ENABLE_PARAMETERS`, see [PROTOCOL.md](../PROTOCOL.md))
lets the host write registered parameters. The app can additionally receive legacy
JSON; KymoCore encodes binary.

## Package structure and checks

```text
KymoCore/
  library.json             PlatformIO metadata and export scope
  library.properties       Arduino metadata
  LICENSE                  MIT license text
  CONTRIBUTING.md          Contributions and grant of rights
  README.md                Overview and quick start
  PROTOCOL.md              Normative wire contract
  CHANGELOG.md             Changes and migration
  PUBLISHING.md            Release preparation for maintainers
  src/
    KymoCore.h             The one header applications include
    kymo_config.h          Version, profile, switches and sizes
    kymo_types.h           Status codes and shared callback types
    telemetry/             Codec, Init/Main runtime, C++ sender
    trace/                 Recorder, trace protocol, instrumentation
    transport/             Mux for trace + DATA on one link
    common/                Bit, byte and CRC helpers
  examples/                Portable and Arduino reference examples
  test/                    Native tests and structural check
  tools/configurator/      Configurator: guided and professional mode
  docs/                    Guide (English/German) and images
```

Run the complete host suite from the repository root:

```sh
python test/native_test.py
python tools/test_package.py
```

`native_test.py` builds and runs the common utilities, protocol golden vectors
with and without CRC, the telemetry runtime in four configurations, the C++
sender, the C entry header for every profile, the portable example, the trace
recorder/mux suite, the compile-out suite and the single-return structural
check. The package check builds separate consumers from the exported archive
and compiles the README example for Uno and ESP32. Builds do not replace board
tests, stack measurements or timing measurements in the driver. Validate your
own adapters on the target hardware. Package preparation is described in
[PUBLISHING.md](../PUBLISHING.md) (German), changes in
[CHANGELOG.md](../CHANGELOG.md).

## Technical API reference

The following reference specifies compiler, configuration and ownership contracts.

Include `KymoCore.h`. The implementation is built as C++11 or newer, without
heap allocation, exceptions or RTTI. All C entry points keep C linkage, so C99
applications and callbacks can use the library. Link mixed C/C++ applications
with the C++ toolchain. PlatformIO and the Arduino IDE compile the `src/`
subfolders automatically; custom builds compile every `src/**/*.cpp` with
`-std=c++11 -fno-exceptions -fno-rtti` and add `src/` to the include path.

## Status codes

| Status | Meaning |
| --- | --- |
| `KYMO_OK` | Success: configuration accepted, frame completely accepted, event copied, record queued |
| `KYMO_IDLE` | Nothing to do: no channel due, no event queued, mux has nothing to send |
| `KYMO_BUSY` | Transport busy, frame partially written or single slot occupied; call again |
| `KYMO_SKIPPED` | Sample callback returned zero |
| `KYMO_BAD_SAMPLE` | Non-finite coordinate, unsupported flags or invalid frame |
| `KYMO_BAD_CONFIG` | Invalid argument/configuration or uninitialized object |
| `KYMO_IO_ERROR` | Impossible write count; fault latched until repair and reinit |
| `KYMO_REJECTED` | Request refused in the current state (e.g. trace context still open) |

`KYMO_OK` from Main means the driver accepted the frame, not physical delivery.

## Minimal build and optional features

The default build contains scalar Y encoding and the Init/Main runtime.
Optional implementations are excluded by the preprocessor, not merely disabled
at runtime. Set `KYMO_ENABLE_RUNTIME=0` for an encoder-only build; add
`KYMO_ENABLE_CPP=1` for a push sender without scheduler. With `KYMO_ENABLED=0`
the library contributes no symbols at all; guard application-owned telemetry
resources with the `KYMO_EFFECTIVE_*` macros and use the `KYMO_TRACE_*` macros,
which compile to no-ops. Unguarded direct API calls deliberately fail to
compile in an off build.

Disabled X/Z/time flags return zero from `KymoProtocol_Encode`,
`KYMO_BAD_CONFIG` from Init and `KYMO_BAD_SAMPLE` from decoding and from the
C++ sender. Fields are never silently removed. `KYMO_ALLOWED_FLAGS` describes
the wire protocol; `KYMO_SUPPORTED_FLAGS` describes this build. KymoStudio
accepts all wire layouts independently of firmware feature selection.

Buffer bounds, finite-value checks, short-write handling and asynchronous buffer
ownership remain mandatory. The maximum frame stays 21 bytes; the switches
reduce code, `KYMO_MAX_CHANNELS` sizes the context.

## Runtime configuration

```c
#include <KymoCore.h>

static uint32_t clock_ms(void *user);  /* your monotonic millisecond clock */
static uint8_t read_sample(void *user, uint8_t id, KymoSample *out);
static uint8_t write_bytes(void *user, const uint8_t *bytes, uint8_t length);

static const KymoChannel channels[] = {
    /* period_ms, id, flags */
    {20, 0, 0},
    {100, 1, 0}
};
static KymoContext kymo;

KymoStatus kymo_setup(void)
{
    KymoConfig config = {0};             /* copied by Kymo_Init */
    config.channels = channels;          /* required */
    config.channel_count = KYMO_COUNT_OF(channels);
    config.clock_ms = clock_ms;          /* required */
    config.sample = read_sample;         /* required */
    config.write = write_bytes;          /* required */
    /* optional: busy, service, clock_user, sample_user, transport_user */
    return Kymo_Init(&kymo, &config);
}

/* From your existing cyclic task: */
void kymo_cycle(void) { (void)Kymo_Main(&kymo); }
```

`read_sample` assigns `out->value` and optional `out->x` / `out->z`; returning
zero skips a measurement until its next interval. It can read shared
application parameters through `sample_user`. The runtime zero-initializes all
sample fields before calling it. Hardware handles belong in `transport_user`;
clocks can use `clock_user`.

`Kymo_Init` copies the configuration, so the `KymoConfig` object may be a local
variable. The channel table and all user contexts must stay alive and
unchanged. Every instance needs its own `KymoContext`, which also holds the
per-channel schedule. Hardware setup remains the application's responsibility.

## Scheduling and memory

- 1–`KYMO_MAX_CHANNELS` channels (up to 256) with unique byte IDs.
- Periods are uint32 milliseconds, limited to INT32_MAX; zero means every
  eligible Main invocation. First samples are due immediately.
- One fixed **21-byte TX buffer** and **4 bytes of state per possible channel**
  inside each context, plus the configuration copy. No measurement queue or heap.
- At most one sample callback and one transport write per Main; at most N
  channel checks. Round-robin selection avoids starvation. Call Main faster
  than the sum of configured sample rates; short writes need additional calls.
- Slow transports apply backpressure. A pending frame retains its original
  value/timestamp; after it finishes, overdue channels sample current values.
  There is no attempt to replay missed samples or an unbounded catch-up burst.
- uint32 subtraction handles clock rollover. Invoke Main at least once within
  INT32_MAX ms. Wire timestamps wrap after approximately 49.7 days.
- All entry points are single-task/non-reentrant. Synchronize application values
  shared with interrupts. Do not reinitialize while DMA/USB borrows a buffer.
- Explicit byte encoding avoids packed structs, enum-width and endian
  dependencies. IEEE-754 binary32 floats are required; do not enable
  finite-math-only/fast-math modes.

## Transport callbacks

`write(user, bytes, length)` returns **accepted bytes**, from zero to length.
Zero means retry later. Short writes resume at their remaining offset. A driver
must never consume bytes it reports as unaccepted. More than length is a
contract error: Main latches IO_ERROR until reinitialization after driver reset.

Synchronous drivers copy/consume bytes before returning. Asynchronous UART/DMA
or CubeMX CDC may borrow the pointer only with a `busy(user)` callback that
stays nonzero until completion. Main neither rewrites nor advances the pending
buffer while busy. A disconnected driver can also report busy. Never mix other
producers into the same byte stream while a frame is partially written.

Optional `service(user)` runs once at the beginning of every valid Main call,
including busy/faulted calls. Use it to maintain network connections. All
callbacks need bounded execution for real-time use; the core cannot make a
blocking driver asynchronous. The PubSubClient example has synchronous network
connection attempts and is therefore not suitable for hard real-time loops.

UDP/MQTT/CAN-FD adapters must accept an entire frame or return zero; individual
messages cannot contain arbitrary frame fragments. UDP gives no delivery or
order guarantee: a lost datagram loses its frames, never the following ones. TCP/UART/USB streams allow
partial writes. CAN-FD padding must be removed before decoding a complete frame
in an adapter; Classic CAN uses the separate mapping described in PROTOCOL.md.

| Main result | Meaning (see Status codes) |
| --- | --- |
| KYMO_OK | Complete frame accepted by driver; not a delivery acknowledgement |
| KYMO_IDLE | No sample due |
| KYMO_BUSY | Transport busy or a frame remains partially written |
| KYMO_SKIPPED | Sample callback returned zero |
| KYMO_BAD_SAMPLE | Non-finite required coordinate; measurement skipped |
| KYMO_BAD_CONFIG | Null/uninitialized context or invalid Init configuration |
| KYMO_IO_ERROR | Impossible write count; fault latched until repair/reinit |

## Optional C++ push sender

Compile with `KYMO_ENABLE_CPP=1` and enable the desired X/Z/time features.
Derive a `KymoStream` adapter for your transport and hand it to `KymoSender`:

```cpp
class UartStream : public KymoStream {
public:
    size_t write(const uint8_t *data, size_t length) override { /* driver */ }
};
static UartStream uart;
static KymoSender sender(uart);

void setup() { sender.setClock(read_clock_ms); }      /* KymoClockFn */
void loop()
{
    if (sender.flush() == KYMO_OK) {                   /* previous frame done */
        (void)sender.send(0, read_sensor(), true);     /* value + timestamp */
    }
}
```

`send`, `send2D` and `send3D` return `KYMO_OK` once the point is accepted into
the single frame buffer. Afterwards call `flush()` cyclically until it returns
`KYMO_OK`. While a frame is pending, new points are rejected with `KYMO_BUSY`.
Invalid data or disabled fields return `KYMO_BAD_SAMPLE`, a missing stream
`KYMO_BAD_CONFIG`, a driver over-report latches `KYMO_IO_ERROR` until `begin()`.

`KymoStream::busy()` defaults to false, requiring write to copy the data.
Override it for borrowed buffers. `KymoSender` is non-copyable; stop transfers
before `begin()` or destruction. The cyclic C API handles scheduling
automatically and is recommended for new integrations.

## Execution tracing and the transport mux

With `KYMO_ENABLE_TRACE=1` (or `KYMO_PROFILE_TRACE`), `KymoTraceRecorder`
records scope, marker, ISR, task and flow events into fixed per-core rings,
and `KymoMux` sends them, administrative records and telemetry frames over a
single link. Only the recorder clock is required:

```c
static KymoTraceRecorder recorder;
static KymoMux mux;
static KymoMuxDataSlot slot;               /* runtime -> mux hand-over */

void trace_setup(void)
{
    KymoTraceConfig trace = {0};
    KymoMuxConfig link = {0};
    KymoConfig telemetry = {0};
    trace.clock_ticks = read_cycle_counter;          /* required */
    (void)KymoTrace_Init(&recorder, &trace);
    (void)KymoMuxDataSlot_Init(&slot);
    link.recorder = &recorder;                       /* required */
    link.write = uart_write;                         /* required */
    link.next_data = KymoMuxDataSlot_NextData;
    link.data_user = &slot;
    (void)KymoMux_Init(&mux, &link);
    /* ...channels, clock and sample as above... */
    telemetry.write = KymoMuxDataSlot_Write;
    telemetry.busy = KymoMuxDataSlot_IsBusy;
    telemetry.transport_user = &slot;
    (void)Kymo_Init(&kymo, &telemetry);
}

void cycle(void)
{
    KYMO_TRACE_SCOPE(&recorder, 1u, SYMBOL_CYCLE);   /* C++; or ENTER/EXIT */
    (void)Kymo_Main(&kymo);
    (void)KymoMux_Service(&mux);
}
```

`core_id`, `current_context` and the critical-section pair are optional.
Without critical callbacks the application serializes all recorder access.
The wire format is described in [TRACE_PROTOCOL.md](TRACE_PROTOCOL.md).

## Platform independence

The entire library, including C++ wrappers, has no vendor/SDK dependency or
conditional target API. It knows only standard C/C++ types, configured
callbacks and the abstract `KymoStream`. Hardware adapters belong to the
application. Each small Arduino example keeps its callbacks in its own
self-contained sketch. Larger transport, performance and trace applications
live in KymoProbe; they are not part of this library package.

## Compatibility and verification

The [wire contract](../PROTOCOL.md) is **v6.2 / wire version 1** and is
unchanged by KymoCore 7.0. CRC calculation is **disabled by default**.
Descriptor bit 0 marks NO_CRC; the final byte remains present as a zero
trailer, keeping frames at 9–21 bytes. Decoders ignore that trailer for NO_CRC
and still validate CRC frames.

To enable sender CRC, compile the library with `-DKYMO_ENABLE_CRC=1` (PlatformIO:
`build_flags = -DKYMO_ENABLE_CRC=1`). A define only in the application source
does not configure separately compiled library code. Use the updated KymoStudio
for the default mode; older receivers require CRC enabled.

`python test/native_test.py` tests the minimal build and the profiles,
including symbol absence for disabled modules, rejection of invalid switch,
profile and size values, codec bounds/CRC/flags, round-robin scheduling,
short writes, busy transports, fault latching, configuration copy and the
trace recorder/mux wire vectors.

## Migrating from KymoCore 6.x

KymoCore 7.0 changes structure and names, not the wire format or features.

| 6.x | 7.0 |
|---|---|
| `#include <kymo_runtime.h>` etc. | `#include <KymoCore.h>` |
| `kymo_build_config.h`, `kymo_features.h`, `kymo_version.h` | `kymo_config.h` |
| `KymoConfig.last_sample_ms` + own array | removed; state lives in `KymoContext` (`KYMO_MAX_CHANNELS`) |
| `KymoConfig` must outlive the context | copied by `Kymo_Init`; may be local |
| Positional `KymoConfig` initializer | field order changed; assign fields by name |
| `KymoProtocol_Encode` / `KymoProtocol_Decode` | `KymoProtocol_Encode` / `KymoProtocol_Decode` (returns `KymoStatus`) |
| `kymo_frame_length` / `kymo_crc8` | `KymoProtocol_FrameLength` / `KymoProtocol_Crc8` |
| `Kymo`, `kymo.h`, `kymo_stream.h` | `KymoSender`, `telemetry/kymo_sender.h` (includes `KymoStream`) |
| `send*()` / `flush()` return `bool` | return `KymoStatus` (`KYMO_OK` = accepted/complete) |
| `setMillisecondCallback(uint32_t (*)())` | `setClock(KymoClockFn, void *user)` |
| `KymoTrace_Init/Pop/ContextReset` return 1/0 | return `KymoStatus` (`KYMO_OK`, `KYMO_IDLE`, `KYMO_REJECTED`) |
| `KymoTraceMux*`, `kymo_transport_mux.h` | `KymoMux*`, `transport/kymo_mux.h` |
| `KymoTraceMux_Service` returns 0/1/2 | returns `KYMO_IDLE`/`KYMO_BUSY`, `KYMO_OK`, `KYMO_IO_ERROR` |
| `KymoTraceMuxConfig.write_frame/is_busy/user` | `write`, `busy` (optional), `transport_user`, plus `data_user` |
| `KymoTraceDataAdapter_*` | `KymoMuxDataSlot_*` |
| `KYMO_TRACE_ADMIN_RECORD_MAX` | `KYMO_MUX_ADMIN_RECORD_MAX` |
| `KymoTraceConfig.core_id/current_context` required | optional (NULL = core 0 / unknown context) |
| `KYMO_CONFIG_API_VERSION` 1 | 2 |

## Coding rules and shared utilities

All active first-party C/C++ functions have at most one return, as their final
statement at function-body scope. Void functions may have no return. Error
paths use initialized status variables and small helpers; no exit macros or
goto-based substitutes. The source rule is checked automatically by the native
test runner and hence CI. Archived prototypes and the independent Events
submodule are outside the active production scope.

The independent [common component](../src/common/README.md) provides reusable
bit/bitset operations, little-endian byte conversion, CRC8, CRC-16 and CRC-32. Constant
mask combinations use `EMB_U8_OR`; runtime helpers use typed static inline
functions with checked shift counts. They are bundled in the package, requiring
no separate framework or library installation. Macros are not assumed to be
faster than compiler-inlined functions.

The C decoder commits to the output structure only after complete validation;
invalid input leaves it unchanged. These conventions improve consistency and
reviewability, but do not constitute a safety-standard compliance claim.
