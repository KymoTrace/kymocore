# Portable Init/Main example

This example runs on a host with a C++11 compiler. It uses the default features:
one scalar channel, no timestamp, no outgoing CRC. Ten `Kymo_Main` calls
generate ten frames using a simulated 20 ms clock step. The transport callback
copies each frame into application storage before returning; it does not retain
the library's buffer. There is no terminal output. Exit code zero means success.

From the unpacked **library root**:

```sh
g++ -std=c++11 -fno-exceptions -fno-rtti -Isrc examples/basic/main.cpp src/telemetry/kymo_protocol.cpp src/telemetry/kymo_runtime.cpp -o kymo-basic
./kymo-basic
```

On Windows, name the output `kymo-basic.exe` and run `.\kymo-basic.exe`.
The compiler must be on PATH. No other files
are required. Keep `KYMO_ENABLE_RUNTIME=1`.

For a device, keep the channel table and context alive (the configuration is copied by Init), replace
the clock/sample/write callbacks and call Init once, then Main from the cyclic
task. The complete Arduino/ESP32 application is in the package README. SDK-based
adapters remain application code; the packaged example itself is portable.
