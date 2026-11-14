# ESP-IDF UART telemetry

Streams a 0.5 Hz sine (channel 0) and a 1 Hz sawtooth (channel 1), each every
20 ms, over UART0 to [KymoStudio](https://kymotrace.de/kymostudio.html).
Plain ESP-IDF, no Arduino core.

```sh
idf.py set-target esp32
idf.py build flash
```

Close `idf.py monitor`, then open the board's serial port in KymoStudio at
115200 baud, 8N1. The binary frames are detected automatically. Logging is
switched off because UART0 also carries the IDF console; boot messages before
`app_main` are skipped by the receiver.

Replace `readSample` in `main/main.cpp` with your measurements and extend
`sChannels` (period in ms, channel ID, flags). The write callback only fills the
free UART FIFO, so `KymoRuntime_main` never waits for the interface.

Inside the repository, `main/idf_component.yml` builds the local KymoCore via
`override_path`. ESP-IDF names that component after its folder, so the
repository folder must be called `kymocore` (the `git clone` default). A project created with
`idf.py create-project-from-example "codename-666/kymocore:uart_telemetry"`
uses the published component instead.
