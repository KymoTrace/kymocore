# Arduino quickstart

Plots the raw ADC readings of A0 and A1 on channels 0 and 1, 115200 baud/8N1.
You declare two `float` variables, list them next to the channel table and
update them in `loop()`. `Kymo_SampleBound` reads them when a channel is due,
so there is no sample callback to write.

Everything lives in `quickstart.ino`:

- **Arduino adapter** (top): millisecond clock and a bounded `Serial` write.
  Keep it as it is on Arduino boards; replace it for other targets.
- **Application** (bottom): variables, channel table, `setup()` and `loop()`.

The sketch needs no build flags and also opens in the Arduino IDE.
From the KymoCore root: `pio run -d examples/arduino/quickstart -e uno`.
Other environments: `nano`, `mega`.

The two tables must have the same order: `values[i]` is plotted on
`channels[i]`. Update the variables from the code that calls `Kymo_Main()`,
not from an interrupt.

## With device timestamps

Without timestamps KymoStudio uses the time of reception. For time-based
analysis let the device stamp each value:

1. Add `build_flags = -DKYMO_ENABLE_TIMESTAMP=1` to `platformio.ini`.
2. Set the channel flags: `{20U, 0U, KYMO_FLAG_TIMESTAMP}`.

[simple_analog_example](../simple_analog_example/README.md) shows this
combination with a hand-written sample callback for computed values.
