# Embedded common utilities

Independent header-only C99/C++11 component, bundled with KymoCore so the
package stays self-contained. No Kymo types, Arduino/HAL headers, heap or
global mutable state. Include individual headers or copy this directory into
another ECU project.

- `embedded_bits.h`: constant-expression `EMB_U8_OR`, typed mask predicates,
  checked byte-bit selection and capacity-checked bitset access.
- `embedded_bytes.h`: unaligned little-endian uint16/uint32/uint64/binary32 reads and writes,
  using memcpy for float bit conversion rather than pointer aliasing.
- `embedded_crc.h`: MSB-first CRC8 with configurable polynomial and initial
  value (the protocol layer selects CRC-8/ATM), CRC-16/CCITT-FALSE and CRC-32 (IEEE)
  for the trace envelope and administrative records.

Constant masks use a macro because static C initializers require constant
expressions. It evaluates each argument once; operands must not have mutually
dependent side effects because C does not order their evaluation. Runtime
operations use typed `static inline` functions, allowing compiler optimization
without macro repetition. Inlining and runtime speed are compiler decisions;
this design does not assume that macros are inherently faster.

Bit indices outside 0–7 yield zero. Bitset helpers check byte capacity and null
pointers before access. Raw byte-conversion helpers require a valid region of
the accessed width; the caller validates it once. Float conversion requires
IEEE-754 binary32. CRC helpers treat null data as an empty input.
Every non-void function has exactly one final return; void helpers have none.

Tests: `test/common_test.c`, compiled through `test/native_test.py`.
