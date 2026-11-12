/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @brief Explicit little-endian integer and binary32 conversion.
 *
 * @details Raw helpers require valid two-, four- or eight-byte spans and do not perform bounds/null checks.
 * Unaligned byte addresses are supported. Float conversion uses memcpy to avoid
 * strict-aliasing violations; the protocol layer validates finite measurements.
 * @file embedded_bytes.h
 * @defgroup embedded_bytes Common byte conversion
 * @{
 * @par Usage
 * Validate the buffer capacity once, then read/write fixed-width payload fields.
 */
#ifndef EMBEDDED_COMMON_BYTES_H
/**
 * @brief Include guard for embedded_bytes.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define EMBEDDED_COMMON_BYTES_H

#include <float.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Compile-time assertion that float occupies exactly four bytes.
 *
 * @details A negative array size rejects unsupported representations. FLT_RADIX, FLT_MANT_DIG
 * and FLT_MAX_EXP checks below additionally require IEEE-754 binary32 characteristics.
 * @par Usage
 * Internal type used when this header is compiled; never instantiate it.
 */
typedef char emb_requires_32_bit_float[(sizeof(float) == 4) ? 1 : -1];
#if FLT_RADIX != 2 || FLT_MANT_DIG != 24 || FLT_MAX_EXP != 128
#error "Float wire conversion requires IEEE-754 binary32"
#endif

/**
 * @brief Write one uint16 in little-endian order.
 *
 * @details Writes exactly two bytes, independent of host endianness; no alignment is required.
 * @param[out] output Non-null writable span of at least two bytes.
 * @param[in] value Unsigned 16-bit value to serialize.
 * @pre The supplied span is valid for the full two-byte access.
 * @par Usage
 * emb_write_u16_le(raw, 0x1234u) writes bytes 34 12 into a two-byte array.
 */
static inline void emb_write_u16_le(uint8_t *output, uint16_t value)
{
    output[0] = (uint8_t)(value & 0xffu);
    output[1] = (uint8_t)(((uint32_t)value >> 8u) & 0xffu);
}

/**
 * @brief Write one uint32 in little-endian order.
 *
 * @details Writes exactly four bytes, independent of host endianness; no alignment is required.
 * @param[out] output Non-null writable span of at least four bytes.
 * @param[in] value Unsigned 32-bit value to serialize.
 * @pre The supplied span is valid for the full four-byte access.
 * @par Usage
 * emb_write_u32_le(raw, 0x12345678u) writes bytes 78 56 34 12 into a four-byte array.
 */
static inline void emb_write_u32_le(uint8_t *output, uint32_t value)
{
    output[0] = (uint8_t)(value & UINT32_C(0xff));
    output[1] = (uint8_t)((value >> 8u) & UINT32_C(0xff));
    output[2] = (uint8_t)((value >> 16u) & UINT32_C(0xff));
    output[3] = (uint8_t)((value >> 24u) & UINT32_C(0xff));
}

/**
 * @brief Write one uint64 in little-endian order.
 *
 * @details Writes exactly eight bytes, independent of host endianness; no alignment is required.
 * @param[out] output Non-null writable span of at least eight bytes.
 * @param[in] value Unsigned 64-bit value to serialize.
 * @pre The supplied span is valid for the full eight-byte access.
 * @par Usage
 * emb_write_u64_le(raw, session_id) writes the least significant byte first.
 */
static inline void emb_write_u64_le(uint8_t *output, uint64_t value)
{
    emb_write_u32_le(output, (uint32_t)(value & UINT64_C(0xffffffff)));
    emb_write_u32_le(output + 4u, (uint32_t)(value >> 32u));
}

/**
 * @brief Read one little-endian uint32.
 *
 * @details Reads exactly four bytes without pointer casts or alignment requirements.
 * @param[in] input Non-null readable span of at least four bytes.
 * @return Decoded unsigned 32-bit value.
 * @pre The supplied span is valid for the full four-byte access.
 * @par Usage
 * uint32_t milliseconds = emb_read_u32_le(frame + offset);
 */
static inline uint32_t emb_read_u32_le(const uint8_t *input)
{
    uint32_t result = (uint32_t)input[0] | ((uint32_t)input[1] << 8u) |
                      ((uint32_t)input[2] << 16u) | ((uint32_t)input[3] << 24u);
    return result;
}

/**
 * @brief Write the IEEE-754 binary32 representation of a float.
 *
 * @details Writes exactly four bytes. No finite-value check is performed; NaN and infinity
 * bit representations can be serialized by this low-level helper.
 * @param[out] output Non-null writable span of at least four bytes.
 * @param[in] value Float whose representation is written.
 * @pre The supplied span is valid for the full four-byte access.
 * @par Usage
 * emb_write_f32_le(payload, -2.5f) writes bytes 00 00 20 C0 into a four-byte array.
 */
static inline void emb_write_f32_le(uint8_t *output, float value)
{
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    emb_write_u32_le(output, bits);
}

/**
 * @brief Read a float from four little-endian binary32 bytes.
 *
 * @details Preserves the represented value, including NaN/Inf. Callers validate semantic
 * constraints separately; the Kymo decoder rejects non-finite coordinates.
 * @param[in] input Non-null readable span of at least four bytes.
 * @return Float represented by the input bits.
 * @pre The supplied span is valid for the full four-byte access.
 * @par Usage
 * float value = emb_read_f32_le(payload);
 */
static inline float emb_read_f32_le(const uint8_t *input)
{
    uint32_t bits = emb_read_u32_le(input);
    float result = 0.0f;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

/** @} */
#endif /* EMBEDDED_COMMON_BYTES_H */
