# Small KymoCore reference examples

KymoCore includes four small reference examples. Each keeps its telemetry
setup in a self-contained source file: include `KymoCore.h`, describe
the channels, provide clock/sample/write callbacks, fill a `KymoConfig` by name,
then call `Kymo_Init` once and `Kymo_Main` cyclically.

| Example | Purpose | Build from the repository root |
|---|---|---|
| [basic](basic/README.md) | Portable C Init/Main without a hardware SDK | See its README |
| [simple analog](arduino/simple_analog_example/README.md) | Two Arduino UART waveform channels | `pio run -d examples/arduino/simple_analog_example -e uno` |
| [multi sensor](arduino/multi_sensor_example/README.md) | Independently scheduled Y/XY/XYZ channels | `pio run -d examples/arduino/multi_sensor_example -e uno` |
| [ESP-IDF UART](esp_idf/uart_telemetry/README.md) | Native ESP-IDF UART without Arduino | See its README |

Replace synthetic samples with sensor acquisition. UART uses 115200 baud/8N1;
configure KymoStudio accordingly. PlatformIO uses the local repository library
and sets the required feature flags for both application and library. Other AVR
environments are `nano` and `mega`.

Larger UART, MQTT, TCP, maximum-throughput, CRC/decoder/C++ and trace/flow demos
belong to [KymoProbe](https://github.com/KymoTrace/kymoprobe), where a central
`src/main.cpp` selects the application. Library changes belong only to KymoCore.
