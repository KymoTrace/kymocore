# Embedded coding conventions

Applies to active first-party C/C++ code, including examples and native tests.

- Every function has at most one `return`, as its last statement at function
  body scope. Void functions, constructors and destructors may have none.
- Precede function definitions in C/C++ sources and Arduino sketches with a
  three-line block-comment separator: an asterisk rule, the function name
  (class-qualified for out-of-class methods), and a closing asterisk rule.
  Keep separator rules at 80 columns including indentation; preserve Doxygen.
- Use explicit initialized result/status variables and small helper functions
  for error paths. Do not replace early returns with goto or exit macros.
- Reuse `src/common/` for bit masks, bitsets, wire byte conversion
  and CRC. Keep protocol constants/semantics in the protocol layer.
- Use unsigned fixed-width operands for bit operations. Check runtime shift
  counts. Macros must parenthesize operands and evaluate each argument once;
  use typed static inline functions for operations requiring validation.
- Preserve bounded execution, no heap in the core, and explicit transport
  buffer ownership. Do not claim safety-standard compliance from style checks.
- Keep optional KymoCore features behind documented platform-independent
  build switches in kymo_config.h, disabled by default (profiles only change
  defaults). kymo_config.h loads and validates them; explicit compiler
  definitions may override the internal settings. The default core
  is scalar C++11 encoding with a C-compatible API plus Init/Main scheduling and bounded transport handling.
  Compile disabled implementations out, reject unsupported requests explicitly,
  and test minimal and enabled builds. Examples opt into required features.
  Keep buffer bounds and transport-ownership checks mandatory.
- Keep the entire KymoCore package platform-independent, including optional
  C++ wrappers. No SDK headers, vendor types or target-dependent API branches.
  Hardware adapters live in application/example code and use callbacks or
  KymoStream. Target macros must not alter the library's public API/layout.
- Run `python test/native_test.py` (includes the single-return structural check),
  `python ../KymoProbe/tools/test_native.py --app ../KymoStudio` when available, and
  affected PlatformIO targets after changes.
- Document active headers with Doxygen: brief, detailed contracts and Usage for
  macros, types, fields and functions; parameter directions and return semantics
  for callable interfaces. Keep the KymoProbe `Doxyfile` inputs current and run Doxygen there after
  header documentation changes. Generated output belongs under `docs/api/`.
- This checkout is the KymoCore development repository. Make library changes here.
- KymoProbe consumes official release tags through its pinned submodule; do not develop in that consumer checkout.
- Keep only a few small reference examples under `examples/` here.
  Larger transport, maximum-performance and feature demonstrations belong to
  KymoProbe and are selected through its central application main.
  KymoProbe retains its examples against its pinned official release; do not
  change library source or the consumer's library pin for demo development.
- Run `python test/native_test.py`, the configurator pytest suite, and `python tools/test_package.py` before a release.
