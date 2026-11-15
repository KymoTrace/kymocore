# Public Core and feature interfaces

KymoCore 8.0.0 introduces a small default-Core interface and reorganizes all
library sources. This is an **API-breaking major release**: include paths and C
function names have changed. Existing module behavior and explicit-instance
integration remain available under the new names. Wire protocols are unchanged.

## Start here

- [kymo_core.h](../src/kymo_core.h): Core init/main and transport configuration.
- [kymo_core_param.h](../src/kymo_core_param.h): parameter definitions and feature init.
- [Shared callback types](../src/kymo_types.h): read/write, busy, service and clock.
- [Runnable C example](../examples/core_parameters/main.c) and
  [runner instructions](../examples/core_parameters/README.md).

```c
#include <kymo_core.h>
#include <kymo_core_param.h>

/* Persistent buffers, targets and driver callbacks are configured beforehand. */
KymoStatus status = KymoCore_init(&coreConfig);
if (status == KYMO_OK)
{
    status = KymoCoreParam_init(&parameterConfig);
}
/* After successful setup, repeatedly call KymoCore_main() and inspect status. */
```

The public feature header contains its configuration and operations, not receiver,
dispatcher or sender objects. The library owns one static default connection and
one static instance per compiled-in feature. Callbacks and buffers remain application-owned.
No heap, implicit task, runtime registration after Main, reset or reconnect is
introduced. Multiple explicit instances still use the module interfaces.

All five features use the same lifecycle. Include only the feature headers you use:

| Feature | Public header | Initialization | Additional application calls |
|---|---|---|---|
| Plot | [kymo_core_plot.h](../src/kymo_core_plot.h) | `KymoCorePlot_init` | Sample callback, optional `KymoCorePlot_sampleBound` |
| Parameters | [kymo_core_param.h](../src/kymo_core_param.h) | `KymoCoreParam_init` | Parameter table; no cyclic feature call |
| Watch | [kymo_core_watch.h](../src/kymo_core_watch.h) | `KymoCoreWatch_init` | Optional memory-read callback |
| Memory | [kymo_core_memory.h](../src/kymo_core_memory.h) | `KymoCoreMemory_init` | Memory measurement callback, optional `KymoCoreMemory_requestInfo` |
| Trace | [kymo_core_trace.h](../src/kymo_core_trace.h) | `KymoCoreTrace_init` | `enter`, `exit`, `mark`, conditional ISR/task/flow hooks and diagnostics |

[Run the complete five-feature example](../examples/core_features/README.md).

For Trace, place `KymoCoreTrace_enter(contextId, symbolId)` at function entry and
`KymoCoreTrace_exit(contextId, symbolId)` on the exit path. C++ can use a local
`KymoCoreTraceScope` object to pair the calls automatically. The generated symbol
dictionary must describe the IDs and event kinds used. Trace hooks before successful
Init or after a Core fault do nothing; optional hooks are absent when their switches
are off. Stack-pointer sampling keeps its existing build-time switch and board hook.
The default facade exposes no recorder, frame encoder or transport scheduler.

## Source organization

```text
src/
  kymo_core.h
  kymo_core_plot.h
  kymo_core_param.h
  kymo_core_watch.h
  kymo_core_memory.h
  kymo_core_trace.h
  kymo_modules.h               Explicit-instance module umbrella
  kymo_config.h              Build settings
  kymo_config_check.h        Validation and effective gates
  kymo_types.h               Shared contracts
  kymo_version.h
  core/                     Core lifecycle and shared endpoint
    kymo_core.cpp
    kymo_core_endpoint.h/.cpp
    detail/                 Private state/registration declarations
  features/
    parameters/             KymoParam, KymoCoreEndpointParam, KymoCoreParam
    plot/                   KymoRuntime, KymoSender, KymoCoreEndpointPlot
    watch/                  KymoWatch, KymoCoreEndpointWatch
    memory/                 KymoMemory, KymoCoreEndpointMemory
    trace/                  KymoTrace, protocol, instrumentation and binding
  communication/            KymoProtocol, KymoControlRx, KymoTx, KymoMux
  binary/                   Shared bit, byte and CRC primitives
```

The former `endpoint/features`, `control`, `telemetry`, `transport` and
`common` source folders are gone. Feature logic and its shared-endpoint binding
are colocated. No header filename is duplicated within `src`.

## Naming contract

- All C/C++ header/source filenames: lowercase snake_case, e.g.
  `kymo_core_endpoint.h` and `kymo_core_endpoint.cpp`.
- C module prefixes: UpperCamelCase, e.g. `KymoCoreEndpoint`.
- C functions: `ModuleName_lowerCamelAction`, e.g. `KymoCoreEndpoint_main`.
  This also applies to C interfaces implemented in C++ source files.
- C++ classes: UpperCamelCase; class methods: lowerCamelCase.
- Callback types end in `Fn`; callback fields/variables end in `Cbk`.
  Callback implementations follow the same C function naming rule as other functions.
- Arrays/array spans end in `Av`; pointers also use `p`.
- Callback fields come first in public Core configuration structs, followed by data.
- Four-space indentation, Allman control/function braces, and a blank line before
  the next documentation block after declarations, structs, typedefs and macros.
- Fixed example configurations are static constants before the entry function.
- Names state units and meaning: `txBufferSizeBytes`, `parameterCount`.
- Boolean names express a condition, e.g. `isReadOverflowEnabled`.
- Nontrivial limits and values are named constants. Documented golden wire-byte
  arrays in tests/examples intentionally show the exact encoded message.

The shared read and write types are `KymoReadBytesFn` and
`KymoWriteBytesFn`. The writer accepts a byte count without narrowing it;
the legacy 8-bit writer contract remains distinct. Redundant TX/Mux/Endpoint callback aliases are removed; these modules use the
canonical shared types directly.

## Build and lifecycle

Enable `KYMO_ENABLE_CORE=1` consistently for every translation unit. Core enables
its internal endpoint and TX implementation automatically, but does not enable a
feature. For parameters additionally enable `KYMO_ENABLE_PARAMETERS=1` and
`KYMO_ENABLE_DECODER=1`. Disable `KYMO_ENABLE_RUNTIME` for a no-Plot build.

1. Core Init activates no feature and performs no driver calls.
2. Successful feature Init validates its configuration and registers its static state.
3. First Core Main freezes registration.
4. Main runs only initialized features; duplicate/late setup is rejected.
5. Feature setup errors latch a startup fault. The facade does not silently
   continue with a partly configured application.

`KymoCore_init(NULL)` creates an empty, transport-free Core.
`KymoCore_main()` then returns IDLE without I/O. A transport-needing feature
cannot subsequently initialize against that Core: provide transport resources
at Core Init instead. Successful Core Init is not repeatable.

With a feature off, its public facade types/functions, implementation and own
static state are absent. Shared codec/types used by other features may remain. With CORE off, its public functions and default connection state are
absent. A compiled-in but uninitialized feature can reserve static RAM, but is
not processed. The global `KYMO_ENABLED=0` overrides both.

## Migration from the earlier source tree

- `kymo_core.h` is now the small Core interface. Explicit-instance consumers use
  `kymo_modules.h` or their individual module headers.
- `KymoEndpoint_*` becomes `KymoCoreEndpoint_lowerCamelAction`.
- Feature bindings use `KymoCoreEndpointParam_init`, etc.
- `Kymo_Init/Main` becomes `KymoRuntime_init/main`.
- C module actions now start lowercase, e.g. `KymoParam_init`.
- Application data fields use the new naming too: `periodMs`, `pTarget`, `pName`,
  `pUnit`, `startAddress`, `sizeBytes`, `totalSizeBytes`, `pChannelsAv`, `pValuesAv`.
- Plot, Parameter and Memory share one canonical configuration each between
  the public facade and explicit binding. Redundant endpoint config aliases are
  removed. Watch and Trace retain separate internal settings for distinct tasks.
- Existing example/configurator generation paths were migrated to the same
  names. The configurator still generates explicit-instance integration.

The old root `KymoCore.h` is replaced by `kymo_core.h`; update include spelling
even on case-insensitive filesystems.

KymoProbe's pinned release and the original development working directory are
not changed by this branch. Its documentation configuration still describes its
pinned release. This branch contains a standalone Doxygen configuration for
all public Core and feature interfaces.

## Verification commands

```text
python test/native_test.py
python -m pip install -e tools/configurator
python -m pytest tools/configurator/tests -q
python examples/core_parameters/run.py
python examples/core_features/run.py
python tools/test_package.py
doxygen Doxyfile
```

The native suite includes the new public-interface tests, positive C/C++ builds,
negative header-isolation/disabled-interface builds, static-symbol checks, normal
and malformed parameter commands, partial I/O and the existing five-feature
matrix. Firmware compilation is separate from hardware execution.
