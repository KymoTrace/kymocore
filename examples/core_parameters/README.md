# Core + Parameter: public interface

Run `python examples/core_parameters/run.py` from the repository root.
The portable C99 application uses only `kymo_core.h` and `kymo_core_param.h`.
It simulates a PC command setting a value from 100 to 200, and checks that Core
sends a readback. No endpoint, receiver or feature-storage object appears in the
application.

1. Configure shared transport buffers and callbacks.
2. Call `KymoCore_init(&core)`.
3. Call `KymoCoreParam_init(&feature)` with the parameter table.
4. Repeatedly call `KymoCore_main()`; inspect its returned status.

On hardware, replace the two callbacks with bounded, nonblocking driver calls.
`readBytesCbk` fills the supplied buffer; `writeBytesCbk` reads the supplied buffer and returns
the accepted count. A writer that borrows the buffer must also provide `isBusyCbk`.
Callbacks do not call back into Core. Tables, target variables, buffers and
callback contexts must remain valid throughout operation.

Build settings in `kymo_example_config.h` must reach every library translation
unit. The runner supplies `KYMO_USER_CONFIG_HEADER` consistently.
No separate endpoint/TX switch or registration call is needed.

For an empty Core, enable only `KYMO_ENABLE_CORE=1`, disable optional features,
call `KymoCore_init(NULL)` and then `KymoCore_main()`. Main returns IDLE and does
no I/O. A transport-free Core cannot later initialize a feature requiring a
transport; configure resources up front. Successful Init cannot be repeated.
All feature initialization finishes before the first Main call.

Fixed configurations are `static const` objects before Main; callbacks, tables and
buffers are assigned there. Main initializes Core and the feature and then runs the
common loop. All five features have equivalent public initializers; see
[the full example](../core_features/README.md). Explicit-instance interfaces remain
available through `kymo_modules.h`. This is an API-breaking development branch.
