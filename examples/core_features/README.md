# Core with all five features

Run `python examples/core_features/run.py` from the library root. The C99 example
includes only the Core header and the five public feature headers. It uses one
Core Init, one Init per feature and one Core Main per loop iteration. All fixed
configurations and tables are defined before Main as static constants.

Transport and measurement callbacks model the hardware. The demo discards outgoing
bytes while counting them; replace `CoreFeaturesExample_writeBytes` with the board's
UART/USB adapter. Watch addresses are mapped through the read callback, never read
as host memory directly. The parameter request changes the configured target from
100 to 200. Trace metadata and the dictionary use the canonical integration fixture.

The build selects `kymo_example_config.h` for every source file. Feature switches
must match in application and library. Callback contexts, buffers, table entries,
parameter targets and trace descriptions must outlive Core processing.

See [the public API guide](../../docs/CORE_ENTRY.md) for activation, compile-out and
trace instrumentation. The native test suite verifies framed output and disabled
interfaces independently.
