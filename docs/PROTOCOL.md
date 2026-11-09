# Kymotrace Compact Binary Protocol v6.2

This document is the authoritative wire-format contract between embedded
senders and KymoStudio. The primary format is compact binary. KymoStudio still
accepts the v5 JSON format so existing firmware does not need to be upgraded
immediately.

Live-variable and memory operations described for 7.3 belong to the unreleased
development version; see the [documentation scope](README.md#documentation-scope--geltungsbereich).

Receiver project: [KymoStudio](https://kymotrace.de/#download).
See the [integration guide](GUIDE.md) for setup and transport ownership.

![Binary frame and descriptor layout](images/protocol.png)

## Design goals

- 9–21 bytes per measurement
- one write per measurement when the driver accepts a full frame; retries for short writes
- constant-time frame length calculation
- no allocation, `snprintf`, JSON library, or float-to-text conversion on MCU
- deterministic little-endian representation
- recovery after dropped or corrupt stream bytes
- optional CRC protection per measurement, disabled by default

The DATA format transports telemetry from a device to KymoStudio. The optional
parameter back channel and control messages are specified below; they extend the
available operations without changing the DATA layout.

## Binary data frame

```text
Offset  Size  Field
0       1     Sync 0: 0xA5
1       1     Sync 1: 0x5A
2       1     Descriptor
3       1     Channel ID (0..255)
4       0/4   X, IEEE-754 float32 little-endian
4/8     4     Y/value, IEEE-754 float32 little-endian
...     0/4   Z, IEEE-754 float32 little-endian
...     0/4   Relative timestamp in ms (or µs, bit 4), uint32 little-endian
last    1     CRC-8/ATM or zero trailer when NO_CRC is set
```

Payload order is always `X?`, `Y`, `Z?`, `timestamp?`. Y is the only
mandatory numeric field. Optional fields are indicated by the descriptor, so
no length byte is necessary.

### Descriptor

```text
Bit 7..6  Wire version: 01
Bit 5     Message type: 0 (data point)
Bit 4     Timestamp unit: 0 = milliseconds, 1 = microseconds (requires bit 1)
Bit 3     X present
Bit 2     Z present
Bit 1     Timestamp present
Bit 0     NO_CRC: 1 = CRC disabled, 0 = CRC-8/ATM present
```

Valid data descriptors are `0x40` through `0x4F`. Even descriptors retain
the v6.0 CRC-protected layout; odd descriptors disable CRC. Unsupported
versions and message types are rejected. Frame sizes are identical in both modes.

Since v6.2, descriptors `0x52`/`0x53`, `0x56`/`0x57`, `0x5A`/`0x5B` and
`0x5E`/`0x5F` (data message type 01) carry the same layouts with the timestamp
in microseconds. Bit 4 without bit 1 (`0x50`, `0x51`, `0x54`, `0x55`, `0x58`,
`0x59`, `0x5C`, `0x5D`) is invalid. Use one unit per connection: receivers
expect a monotonic device clock and treat a unit change as a clock restart.

### Frame sizes

| Measurement | Without timestamp | With timestamp |
|---|---:|---:|
| Y | 9 bytes | 13 bytes |
| X/Y | 13 bytes | 17 bytes |
| Y/Z | 13 bytes | 17 bytes |
| X/Y/Z | 17 bytes | 21 bytes |

### Numeric rules

- Channel ID is an unsigned byte, 0–255.
- X, Y, and Z are finite IEEE-754 binary32 values. NaN and infinity are invalid.
- Timestamp is an unsigned 32-bit count of milliseconds relative to the sender
  start time. It wraps after approximately 49.7 days. With bit 4 it counts
  microseconds and wraps after approximately 71.6 minutes; KymoStudio continues
  the time axis across a wrap when the next value lies within one second past it.
- Multi-byte values are little-endian regardless of MCU endianness.
- Binary32 intentionally trades decimal text precision for fixed size and fast
  MCU encoding. It provides roughly seven significant decimal digits.

### CRC

CRC is disabled by default. A sender sets descriptor bit 0 (`NO_CRC`) and
writes a zero trailer without calculating CRC. Receivers ignore the trailer
for these frames, including a nonzero trailer received after corruption.
Sync, descriptor, exact length and finite coordinate checks still apply.

For KymoCore, compile the library with `-DKYMO_ENABLE_CRC=1` to enable CRC;
the default is `0`. This applies to the C codec, cyclic runtime and C++ sender.
Define the flag for the library compilation, not only in a consuming sketch.
In PlatformIO use `build_flags = -DKYMO_ENABLE_CRC=1` in the selected environment.
Python encoding uses `encode_data_point(point, crc_enabled=True)` to opt in.
Both decoders accept mixed protected/unprotected frames regardless of encoder mode.
Older v6.0 receivers reject NO_CRC descriptors: update KymoStudio before using
default-mode firmware, or enable CRC for compatibility with those receivers.

CRC-8/ATM parameters:

- polynomial: `0x07`
- initial value: `0x00`
- input/output reflection: false
- xor-out: `0x00`
- covered bytes: descriptor, channel, and payload; sync bytes are excluded

A receiver must discard a protected frame with a bad CRC and search for the
next `A5 5A` sync marker. Unprotected frames have no checksum validation;
corrupt finite values may be delivered and stream resynchronization is weaker.

## Golden vectors

These vectors are shared by the Python and Embedded implementations.

Default mode (CRC disabled):

```text
Point: id=7, y=1.0
Bytes: A5 5A 41 07 00 00 80 3F 00
Size:  9

Point: id=3, x=1.25, y=-2.5, z=9.0, timestamp=1234 ms
Bytes: A5 5A 4F 03 00 00 A0 3F 00 00 20 C0 00 00 10 41 D2 04 00 00 00
Size:  21

Point: id=3, y=-2.5, timestamp=1234567 µs
Bytes: A5 5A 53 03 00 00 20 C0 87 D6 12 00 00
Size:  13
```

CRC enabled (unchanged v6.0 vectors):

```text
Point: id=7, y=1.0
Bytes: A5 5A 40 07 00 00 80 3F 54
Size:  9

Point: id=3, x=1.25, y=-2.5, z=9.0, timestamp=1234 ms
Bytes: A5 5A 4E 03 00 00 A0 3F 00 00 20 C0 00 00 10 41 D2 04 00 00 89
Size:  21

Point: id=3, y=-2.5, timestamp=1234567 µs
Bytes: A5 5A 52 03 00 00 20 C0 87 D6 12 00 97
Size:  13
```

Microsecond timestamps are encoded with `KymoProtocol_Encode` and
`KYMO_FLAG_TIMESTAMP | KYMO_FLAG_TIMESTAMP_US` on a `KymoDataPoint`
(`KYMO_ENABLE_TIMESTAMP=1`); `KymoDataPoint::timestamp_ms` then holds µs.
The cyclic runtime and C++ sender keep millisecond clocks and reject the flag.
Python encoding uses `encode_data_point(point, timestamp_us=True)`.

## Embedded sender interface

The default firmware build supports scalar Y only. Enable `KYMO_ENABLE_X`,
`KYMO_ENABLE_Z` and/or `KYMO_ENABLE_TIMESTAMP` for optional fields, and
`KYMO_ENABLE_CPP` for the C++ sender. The default C Init/Main runtime can
be removed with `KYMO_ENABLE_RUNTIME=0`. MCU decoding is opt-in through
`KYMO_ENABLE_DECODER=1`; it rejects layouts disabled in that firmware build.
The wire layout and the desktop receiver's supported formats are unchanged.
All settings and profiles live in `src/kymo_config.h`, which also validates them.
Compiler definitions may override the header.

KymoCore provides a portable C++11 cyclic API. The v6.1 NO_CRC extension keeps
wire version 1 and all frame sizes. `Kymo_Init(context, config)` validates and copies a configuration;
`Kymo_Main(context)` schedules samples, encodes frames and resumes short
writes. See [GUIDE.md](GUIDE.md) for configuration and transport ownership.

The optional C++ push sender `KymoSender` remains available:

```cpp
sender.send(0, temperature);           // Y, 9 bytes
sender.send2D(1, position, force);     // X/Y, 13 bytes
sender.send3D(2, x, y, z, true);       // X/Y/Z + timestamp, 21 bytes
```

`KYMO_OK` means accepted into the fixed 21-byte buffer. Call `flush()`
cyclically until it returns `KYMO_OK` before submitting another point. Timestamps
in the C++ API require `setClock` and optionally `setStartTime`.
Async transports must report busy until they release the transmitted buffer.

## Throughput

At 115200 baud, UART 8N1 carries approximately 11,520 wire bytes per second.
Ignoring application and driver overhead, this permits approximately:

| Frame | Binary points/s | Equivalent v5 JSON points/s |
|---|---:|---:|
| Y | 1,280 | 443 |
| X/Y/Z + timestamp | 548 | 155 |

The minimal binary frame is 9 bytes versus 26 bytes for the previous formatted
JSON example. The full golden vector is 21 bytes versus 74 bytes as JSON.

## Transport rules

### Serial, USB CDC, and Telnet/TCP

Frames may be fragmented or concatenated arbitrarily. Receivers must use sync,
descriptor-derived length, and CRC when present; packet boundaries from individual reads
have no meaning. JSON compatibility messages remain newline-delimited.

### MQTT

One MQTT payload may contain one or more concatenated binary frames. MQTT
message boundaries are sufficient, but the normal stream decoder is used so
concatenation behaves identically to Serial/TCP.

### CAN

For Classic CAN, the native compact mapping is preferred:

- arbitration ID maps to Kymotrace channel (`data_id` can override it)
- CAN data contains the numeric value in the configured `float32`, `float64`,
  signed, or unsigned format
- scaling and offset are applied by the receiver adapter

This uses only four CAN data bytes for a float32 measurement. CAN-FD may carry
an entire v6 binary frame unchanged when X/Z/timestamp or CRC-compatible
cross-transport framing is required.

## Back channel: host writes to parameters

Since KymoCore 7.1 the host may discover and write registered application
parameters (`KYMO_ENABLE_PARAMETERS=1`, which requires `KYMO_ENABLE_DECODER=1`).
The firmware declares them in one table, for example
`KYMO_PARAM(100, kp, 0.0f, 10.0f, "")`; names, units and IDs live only there.

| Direction | Frame | Meaning |
|---|---|---|
| Host → device | Parameter control `a5 5a 60 01 01 d7` | List request |
| Device → host | Parameter control, operation 0x02, one per parameter | Description |
| Host → device | DATA, CRC required, no X/Z/timestamp; `id` = parameter, `value` = new setting | Write request |
| Device → host | DATA with the same `id`; `value` = what the parameter now holds; optional timestamp | Readback |

### Parameter control frame

Descriptor `0x60`: wire version 1 with bit 5 set marks a non-DATA message, and
bits 4..0 select it (`0x61` is reserved for a later address-based write).

| Offset | Size | Field |
|---:|---:|---|
| 0 | 2 | Sync `A5 5A` |
| 2 | 1 | Descriptor `0x60` |
| 3 | 1 | Payload length L (1..64) |
| 4 | L | Payload; first byte is the operation |
| 4+L | 1 | CRC-8/ATM over descriptor, length and payload (mandatory) |

Operation `0x01` LIST (host → device) has no further fields. Operation `0x02`
DESCRIPTION (device → host) carries `id` (u8), `index` (u8), `count` (u8),
`minimum` and `maximum` (f32 LE), then the name and the unit, each as a length
byte followed by UTF-8 bytes without terminator (name ≤ 24, unit ≤ 8 bytes).
`index` and `count` tell the host when the list is complete.

Golden vectors: list request `a5 5a 60 01 01 d7`; write parameter 7 = 1.0:
`a5 5a 40 07 00 00 80 3f 54`.

### Rules

After a list request the device sends every description and a readback of every
current value. The device applies a write only for a registered ID and a finite
value inside the parameter's inclusive [minimum, maximum]. An accepted or
out-of-range write is answered with a readback, so the host sees the value
actually in force. Unknown IDs, NO_CRC frames, bad CRCs and frames with extra
fields get no readback. The host treats a missing readback as "not confirmed";
it never assumes success.

Parameter IDs share the channel ID space with measurements. Firmware should pick
IDs its measurement channels do not use, so readbacks are distinguishable.

Transports: Serial/USB, TCP and UDP carry the frames unchanged (UDP replies go
to the last sender). MQTT publishes them on the configured `tx_topic`.

CAN uses a compact write instead, because a classic data field holds 8 bytes:
arbitration ID = the connection's configured `tx_id`, data = parameter ID (u8)
followed by the value as little-endian binary32 (5 bytes); CAN's own framing and
CRC replace sync, descriptor and CRC-8. The firmware hands the data field to
`KymoParam_FeedCompact`, which applies the same rules. The readback is an ordinary
CAN measurement whose arbitration ID is the parameter ID. Discovery does not fit
into 8 bytes, so parameters on CAN are defined in the host. Golden vector, write
parameter 7 = 1.0: `07 00 00 80 3f`.

Reserved for later: an address-based write (parameters derived from an ELF
symbol table) uses descriptor `0x61`, so the frames above keep their meaning. It
must stay limited to memory ranges the firmware explicitly allows.

## Live variables: the device samples variables chosen from its ELF file

Since KymoCore 7.3 the host may ask the device to sample global or static
variables by address (`KYMO_ENABLE_WATCH=1`, which requires
`KYMO_ENABLE_DECODER=1`). The host reads names, addresses and types from the ELF
file of the running firmware; the firmware only declares which memory ranges may
be read. Reading only: nothing is ever written through this mechanism.

The messages use the parameter control frame (descriptor `0x60`, mandatory CRC)
with these operations:

| Op | Direction | Payload after the operation byte | Meaning |
|---|---|---|---|
| `0x03` WATCH_INFO | Host → device | – | Stop every slot and report capabilities |
| `0x04` WATCH_INFO_REPLY | Device → host | `slots` u8, `min_period_ms` u16, `id_length` u8 (0..20), build ID, `range_count` u8 (0..4), per range `start` u32 and `length` u32 | Capabilities |
| `0x05` WATCH_SET | Host → device | `slot` u8, `channel` u8, `address` u32, `type` u8, `period_ms` u16 | Sample a variable; period 0 stops the slot |
| `0x06` WATCH_ACK | Device → host | `slot` u8, `status` u8 | Result of one WATCH_SET |

All multi-byte fields are little-endian. Types: 1 u8, 2 i8, 3 u16, 4 i16,
5 u32, 6 i32, 7 f32, 8 f64, 9 bool (one byte, nonzero = 1).

ACK status: 0 accepted, 1 slot out of range, 2 not completely inside an allowed
range, 3 address not aligned to the type size, 4 unknown type, 5 period below
`min_period_ms`. A rejected WATCH_SET leaves the slot stopped.

Samples are ordinary DATA frames on the requested `channel`, with the device
timestamp when the firmware has a clock. The value is converted to binary32, so
integers above 2^24 lose precision. Each slot is read with exactly its type's
width; aligned reads up to 4 bytes are single-copy atomic on 32-bit MCUs, an f64
read may tear unless the firmware installs a lock.

### Rules

- WATCH_INFO stops all slots, so a host that (re)connects starts from a known
  state, then sets every slot it wants.
- Every WATCH_SET is answered by a WATCH_ACK. The host never assumes a slot is
  active without one, and treats a missing answer as "not confirmed".
- The build ID is application data: on ESP32 the first bytes of the ELF file's
  SHA-256 (`esp_app_desc_t::app_elf_sha256`), elsewhere the GNU build ID
  (`-Wl,--build-id`). The host accepts the ELF file when the device's ID is a
  prefix of either; with an empty or different ID it warns and asks before use.
- Channels share the ID space with measurements and parameters; the host picks
  IDs that are not in use.
- Classic CAN cannot carry these frames.

### Golden vectors

```text
WATCH_INFO:                a5 5a 60 01 03 d9
WATCH_INFO_REPLY:          a5 5a 60 12 04 08 05 00 04 de ad be ef 01 00 00 fb 3f 00 c2 02 00 75
  8 slots, minimum 5 ms, build ID de ad be ef, range 0x3FFB0000 + 0x2C200
WATCH_SET:                 a5 5a 60 0a 05 00 c8 34 12 fb 3f 07 14 00 55
  slot 0, channel 200, address 0x3FFB1234, f32, every 20 ms
WATCH_ACK accepted:        a5 5a 60 03 06 00 00 eb
WATCH_ACK out of range:    a5 5a 60 03 06 01 02 f0   (slot 1)
```

## Memory telemetry: stack peaks and heap levels

Since KymoCore 7.3 the device may report its memory as named curves
(`KYMO_ENABLE_MEMORY=1`, no decoder needed). The firmware lists the entries
(task stacks, heaps) and reads the values in a callback; KymoCore does not call
any RTOS or allocator function itself.

Every entry has its own DATA channel. The device describes each entry with a
control frame (descriptor `0x60`, mandatory CRC), unsolicited: right after
start, every `info_period_ms` and when the firmware calls
`KymoMemory_RequestInfo`.

| Op | Direction | Payload after the operation byte | Meaning |
|---|---|---|---|
| `0x07` MEMORY_INFO | Device → host | `channel` u8, `kind` u8, `total` u32, name (0..16 bytes UTF-8, rest of the payload) | Describes one entry |

Kinds: 1 peak stack use in bytes (stack size minus watermark), 2 free heap bytes
now, 3 lowest free heap bytes since start. `total` is the stack or heap size in
bytes, 0 when unknown. Samples are ordinary DATA frames on `channel` with the value
in bytes; the firmware converts words to bytes before reporting.

Hosts name the channel after the entry, e.g. `stack/parser` or `heap/heap`, and
keep showing the last description until a new one arrives. A restart of the device
repeats every MEMORY_INFO.

```text
MEMORY_INFO:               a5 5a 60 0d 07 dc 01 00 10 00 00 70 61 72 73 65 72 7e
  channel 220, peak stack use, total 4096 bytes, name "parser"
```

## Legacy JSON compatibility

KymoStudio continues to accept UTF-8 JSON:

```json
{"id":0,"value":25.5}
{"id":0,"value":25.5,"timestamp":1.234}
{"id":0,"x":10.5,"y":25.5}
{"id":0,"x":10.5,"value":25.5,"z":99.9,"timestamp":1.234}
```

Rules:

- `id`: required integer, 0–255
- `value` or `y`: required finite number
- `x`, `z`: optional finite numbers
- `timestamp`: optional finite, non-negative seconds
- a plain finite number is accepted as legacy channel 0
- stream transports require `\n`; MQTT may use one JSON object per payload

JSON is a compatibility and diagnostics format, not the recommended embedded
format. New embedded firmware should emit binary v6.

## Implementations

- Python codec: `KymoStudio/python/Receiver/binary_protocol.py`
- Backend parser: `KymoStudio/python/Backend/backend.py::_parse_data_point`
- Embedded codec: `src/telemetry/kymo_protocol.h/.cpp`
- Embedded cyclic runtime: `src/telemetry/kymo_runtime.h/.cpp`
- Optional C++ sender: `src/telemetry/kymo_sender.h/.cpp`
- Embedded golden-vector test: `test/protocol_golden_test.cpp`
- Embedded back channel: `src/control/kymo_param.h/.cpp` (parameters),
  `src/control/kymo_watch.h/.cpp` (live variables), tests `test/param_test.cpp`
  and `test/watch_test.cpp`
- Desktop golden-vector tests: `KymoStudio/tests/test_binary_protocol.py`

## Versioning

- Protocol v6.2 / wire version 1: microsecond timestamps via data message type 01
  (descriptor bit 4); unchanged field order and sizes, older receivers reject it
- Protocol v6.1 / wire version 1: NO_CRC descriptor bit, disabled CRC by default;
  unchanged field order and sizes, continued acceptance of protected v6.0 frames
- Protocol v6.0 / wire version 1: compact binary frames; JSON v5 receive fallback
- Protocol v5.0: JSON X/Y/Z format
- Protocol v4.0: JSON timestamp format

Any future incompatible field order or size must use a new wire-version value.
The NO_CRC extension uses the formerly reserved bit without changing the layout;
old receivers reject that bit rather than silently interpreting the new mode.
New message semantics that fit the current layout may use a reserved message
type, but v6 receivers must reject types they do not understand.
