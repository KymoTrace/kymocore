<p align="center">
  <a href="README.md"><img alt="English" src="https://img.shields.io/badge/%F0%9F%8C%90-English-15123A"></a>
  <a href="README.de.md"><img alt="Deutsch" src="https://img.shields.io/badge/%F0%9F%8C%90-Deutsch-A78BFA"></a>
</p>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/images/kymotrace-logo-dark.svg">
    <img src="docs/images/kymotrace-logo-light.svg" alt="Kymotrace – Embedded Telemetry" width="480">
  </picture>
</p>

<h3 align="center">KymoCore · Measurements from your microcontroller, live on screen</h3>

<p align="center">
  Portable C++11 telemetry library for Arduino, ESP32 and STM32 – the heart of <b>Kymotrace</b>.
</p>

<p align="center">
  <img alt="Version 7.2.0" src="https://img.shields.io/badge/version-7.2.0-7C5CFF">
  <img alt="License MIT" src="https://img.shields.io/badge/License-MIT-15123A">
  <img alt="C++11 with C API" src="https://img.shields.io/badge/C%2B%2B11-C--API-A78BFA">
  <img alt="Platforms" src="https://img.shields.io/badge/Arduino%20%C2%B7%20ESP32%20%C2%B7%20STM32-PlatformIO-FDE047">
</p>

---

## What is it about?

Anyone evaluating sensors, tuning a controller or driving a motor wants to
**see what is happening inside the microcontroller right now**: immediately,
not after reading out a log file. `Serial.print` quickly reaches its limits.
Text costs CPU time and bandwidth, blocks the main loop and has to be parsed
laboriously on the PC.

**KymoCore** solves exactly that. The library reads your measurements at the
configured rate, packs them into compact binary frames of **9 to 21 bytes** and
hands them to any interface without holding up the main cycle. On the PC,
[KymoStudio](https://kymotrace.de/#download) shows the signals live
as curves, XY traces or in 3D.

<p align="center">
  <img src="docs/images/kymostudio-workbench.png" alt="KymoStudio showing measurements live as a time series and an XY chart" width="900">
  <br><sub>Live data in KymoStudio: three signals over time, two as XY traces.</sub>
</p>

## What KymoCore can do

### Project configuration and production builds

The optional [Kymo configurator](tools/configurator/README.md) offers a
guided step-by-step setup and a professional mode with every setting, plus a Qt-free
CLI for versioned project profiles. Generated
configuration snapshots can be compiled without running Python or installing Qt.
Compiler definitions override the project header, which overrides library defaults.
Use `KYMO_ENABLED=0` for production and guard application-owned telemetry resources
with the effective feature macros. Include `KymoCore.h` (or `trace/kymo_instrumentation.h`) for compile-out
instrumentation; unguarded low-level API calls deliberately fail in an off build.

| | |
|---|---|
| **Many channels, own rate** | Up to 256 channels, each with its own sampling interval. A round-robin scheduler decides whose turn it is. |
| **More than one number** | Y, XY or XYZ per data point, optionally with a timestamp and CRC checksum. |
| **Compact on the wire** | 9–21 bytes per data point, float32, little endian. A fixed, documented [protocol](PROTOCOL.md). |
| **Never blocks** | Call `Kymo_Init()` once, then `Kymo_Main()` cyclically. Runs without RTOS, heap, exceptions or RTTI. |
| **Any interface** | UART, USB CDC, Wi-Fi/TCP, UDP, MQTT, CAN …: you provide a `write` callback, KymoCore provides the bytes. |
| **Slow links included** | Partial transfers, backpressure and borrowed DMA buffers are handled cleanly. |
| **Only what you need** | X, Z, timestamp, CRC, decoder and the C++ API can each be enabled at compile time. The default is minimal. |
| **C and C++** | C++11 core with a C-compatible API, usable from C99 projects too. |

## What it is used for

- **Control engineering:** compare setpoint, actual value and output of a PID loop live and tune the parameters.
- **Sensor development:** make noise, drift and step response of sensors visible.
- **Motors and power electronics:** record currents, speeds and positions with timestamps.
- **Test benches and endurance runs:** monitor many channels in parallel; enable the optional CRC to detect transmission errors.
- **Robotics and motion:** display paths and trajectories directly as XY or XYZ curves.
- **Teaching, university and maker projects:** make measurement data understandable instead of reading columns of numbers in the serial monitor.

## How it works

![From measurement to chart](docs/images/architecture.png)

1. **Your application** provides the clock and the measurement function and defines which channels are sampled how often.
2. **KymoCore** decides which channel is due, calls your measurement function and encodes the frame.
3. **Your transport** (UART, USB, TCP, UDP, MQTT …) takes the bytes, piece by piece if necessary.
4. **KymoStudio** reassembles the data stream, checks it and draws the curves.

The library deliberately contains **no hardware drivers**. That keeps it the
same on every platform, from the Arduino Uno to an STM32 with DMA.

## Quick start

Add this to your project's `platformio.ini`:

```ini
lib_deps =
    codename666/KymoCore @ 7.2.0
```

### Complete example for Arduino and ESP32

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

Build with `pio run`, then flash the connected board with `pio run -t upload`.
Connect KymoStudio at 115200 baud and 8N1 and assign channel 0 to a chart.
Close any open serial monitor first. A completely SDK-free, executable example
is in [examples/basic](examples/basic/README.md).

## Supported platforms

KymoCore is platform-independent. The larger hardware examples in
[KymoProbe](https://github.com/KymoTrace/kymoprobe) cover these targets:

| Family | Boards | Transport in the example |
|---|---|---|
| Arduino AVR | Uno, Nano, Mega | UART |
| ESP32 | ESP32, ESP32-S3, ESP32-C3 | UART, Wi-Fi/MQTT |
| STM32 | Nucleo F401RE, F411RE, Blue Pill F103C8 | UART, USB CDC |
| PC (native) | GCC/Clang | test programs |

## The protocol at a glance

![Frame layout](docs/images/protocol.png)

The complete specification with example frames is in [PROTOCOL.md](PROTOCOL.md).

## Part of Kymotrace

| Project | Role |
|---|---|
| **[KymoCore](https://github.com/KymoTrace/kymocore)** | this library: capture and send measurements |
| [KymoProbe](https://github.com/KymoTrace/kymoprobe) | firmware and hardware examples for ESP32, Arduino and STM32 |
| [KymoStudio](https://kymotrace.de/#download) | desktop app: receive, display, analyse, export |

## Documentation

- **[Guide and API reference](docs/GUIDE.md):** configuration, scheduling, transports and sizing
- **[Protocol](PROTOCOL.md):** byte layout, CRC, example frames
- **[Portable example](examples/basic/README.md):** runs on the PC without hardware
- **[Small reference examples](examples/README.md):** portable Init/Main and Arduino waveform sketches
- **[Changelog](CHANGELOG.md)** · **[Publishing](PUBLISHING.md)** (German) · **[Contributing](CONTRIBUTING.md)**

## License

Copyright (c) 2026 Christof Seidel. KymoCore is licensed under the
[MIT license](LICENSE): free for any use, including commercial firmware and
devices, without disclosing your own source code. Each release keeps the license
it was published with: 6.2.0 to 7.1.0 under GPLv3 or a commercial license, up to
6.1.0 under MIT.
