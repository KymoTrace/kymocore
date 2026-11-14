# Embedded coding conventions

Applies to active first-party C/C++ code, including examples and native tests.

- Every function has at most one `return`, as its last statement at function
  body scope. Void functions, constructors and destructors may have none.
- Precede function definitions in `.cpp` files with a three-line separator:
  `/` plus 100 asterisks, ` * <function name>` (class-qualified for out-of-class
  methods), and ` ` plus 99 asterisks plus `/`. A description follows as a
  `/* ... */` comment. In `.c`, `.h` and `.ino` files use one block comment
  that opens with `/**` alone on its line, ` * ` lines and a closing ` */`.
  Leave exactly one blank line between the end of a function and the next
  comment block; preserve Doxygen.
- Use explicit initialized result/status variables and small helper functions
  for error paths. Do not replace early returns with goto or exit macros.
- Reuse `src/binary/` for bit masks, bitsets, wire byte conversion
  and CRC. Keep protocol constants/semantics in the protocol layer.
- Use unsigned fixed-width operands for bit operations. Check runtime shift
  counts. Macros must parenthesize operands and evaluate each argument once;
  use typed static inline functions for operations requiring validation.
- Preserve bounded execution, no heap in the core, and explicit transport
  buffer ownership. Do not claim safety-standard compliance from style checks.
- Keep optional KymoCore features behind documented platform-independent
  build switches in kymo_config.h, disabled by default (profiles only change
  defaults). kymo_config.h holds only settings; kymo_config_check.h validates
  them and derives KYMO_EFFECTIVE_*, kymo_version.h holds the version; explicit compiler
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

## Library naming rules

- Callback typedefs end in `Fn`; callback variables/fields end in `Cbk`.
  C callback implementations use the regular `Module_lowerCamelAction` scheme.
- C/C++ filenames use lowercase snake_case with a shared header/source stem.
  Module prefixes use UpperCamelCase; functions use
  `ModuleName_lowerCamelAction`, including C interfaces implemented in .cpp.
  Private helpers use the same module prefix. C++ classes use UpperCamelCase,
  class methods use lowerCamelCase; constructors/destructors follow C++ syntax.
- C configuration structures list all callback fields first, then data parameters.
- Arrays and pointers to array spans end in `Av`; pointers also use the `p` prefix.
- Use descriptive lowerCamelCase names and full parameter names. Boolean names
  begin with `is`, `has`, `can` or `should`; use explicit true/false comparisons
  when testing actual bool values. Status codes and byte counts are not booleans.
- Name sizes by unit (`txBufferSizeBytes`) and counts by contents (`parameterCount`).
- Give nontrivial limits, channel IDs, test bounds and initial/target values named
  constants. Exact documented protocol golden byte arrays are test fixtures,
  not unexplained algorithm constants. Zero/one for initialization and iteration
  do not need artificial aliases.
- Explicit-instance interfaces remain available under migrated names. Keep different
  callback contracts distinct (for example the legacy 8-bit write length).

## Layout and example configuration

- Use Allman braces: opening braces for functions and control blocks are on their
  own line below the controlling statement, with consistent four-space indentation.
- Separate declarations, typedefs, structs and preprocessor definitions from the
  next documentation/comment block by a blank line. Do not join unrelated sections.
- Keep fixed example configurations and tables at file scope as static constants;
  entry functions initialize and process. Assign genuinely runtime-dependent values
  only once those values exist. C examples may use C99 designated initializers;
  C++11 examples use documented aggregate initializers.
- `python tools/format_sources.py` applies .clang-format plus comment spacing.
