# Arduino

Start with `quickstart`: it plots two variables through `Kymo_SampleBound` and
needs no sample callback or build flags.

Use `simple_analog_example` for two waveform channels, or
`multi_sensor_example` for Y/XY/XYZ. Each sketch is self-contained and marks the
four integration steps in its comments: **WHAT** (channel table), **WHEN**
(clock), **VALUE** (sample callback) and **WHERE** (write callback). Replace the
waveform in `readSample()` with `analogRead(A0)` for actual ADC measurements.
Select Uno, Nano or Mega in the project's platformio.ini. Serial connection:
115200, 8N1, KymoStudio binary v6. See [example build instructions](../README.md).

These small examples use only the C Init/Main API. The larger C++ sender,
network, performance and trace demonstrations live in KymoProbe.
