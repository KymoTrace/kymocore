/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @brief Reusable byte masks and bounded bitset operations.
 *
 * @details Header-only C99/C++11 helpers with unsigned arithmetic and no global state.
 * Functions evaluate arguments once; callers synchronize shared mutable bitsets.
 * @file embedded_bits.h
 * @defgroup embedded_bits Common bit operations
 * @{
 * @par Usage
 * Include common/embedded_bits.h and use masks instead of duplicating shifts.
 */
#ifndef EMBEDDED_COMMON_BITS_H
/**
 * @brief Include guard for embedded_bits.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define EMBEDDED_COMMON_BITS_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Combine two byte masks in a constant expression.
 *
 * @details Both operands are converted to uint8_t; bits above bit seven are discarded.
 * Each argument occurs once, but operand evaluation order is unspecified. Do not
 * pass arguments with mutually dependent side effects.
 * @param[in] left First byte-compatible mask expression.
 * @param[in] right Second byte-compatible mask expression.
 * @return Byte-sized bitwise union of the operands.
 * @par Usage
 * static const uint8_t flags = EMB_U8_OR(0x08u, 0x02u);
 */
#define EMB_U8_OR(left, right) ((uint8_t)((uint8_t)(left) | (uint8_t)(right)))

/**
 * @brief Extract only selected bits from a byte.
 *
 * @details Bitwise AND with no side effects; unselected bits are cleared.
 * @param[in] value Input byte.
 * @param[in] mask Selection mask: one keeps a bit, zero clears it.
 * @return The masked byte, not a boolean.
 * @par Usage
 * uint8_t flags = emb_u8_mask(descriptor, 0x0e);
 */
static inline uint8_t emb_u8_mask(uint8_t value, uint8_t mask)
{
    uint8_t result = (uint8_t)(value & mask);
    return result;
}

/**
 * @brief Test whether at least one selected bit is set.
 *
 * @details An empty mask always produces zero.
 * @param[in] value Byte to inspect.
 * @param[in] mask Bits whose presence should be checked.
 * @return One if any selected bit is set; zero otherwise.
 * @par Usage
 * Test emb_u8_has_any(flags, KYMO_FLAG_X) before accessing an optional X value.
 */
static inline uint8_t emb_u8_has_any(uint8_t value, uint8_t mask)
{
    uint8_t result = (uint8_t)(emb_u8_mask(value, mask) != 0u);
    return result;
}

/**
 * @brief Check that no bit outside an allowed set is present.
 *
 * @details The all-zero input is valid for any allowed mask. No input is modified.
 * @param[in] value Candidate flag byte.
 * @param[in] allowed Union of all permitted bits.
 * @return One when value is a subset of allowed; zero when unsupported bits exist.
 * @par Usage
 * Validate flags before serialization.
 */
static inline uint8_t emb_u8_only_bits(uint8_t value, uint8_t allowed)
{
    uint8_t result = (uint8_t)(emb_u8_mask(value, (uint8_t)~allowed) == 0u);
    return result;
}

/**
 * @brief Create a one-bit byte mask with a checked shift count.
 *
 * @details Indices 0..7 select least- to most-significant bits. Larger indices return zero
 * without performing a shift, avoiding shifts outside the intended byte range.
 * @param[in] index Unsigned bit position; values 8..255 are rejected.
 * @return One of 0x01..0x80 for valid indices; zero otherwise.
 * @par Usage
 * uint8_t high_bit = emb_u8_bit(7);
 */
static inline uint8_t emb_u8_bit(uint8_t index)
{
    uint8_t result = 0;
    if (index < 8u)
    {
        result = (uint8_t)(UINT32_C(1) << index);
    }
    return result;
}

/**
 * @brief Read one bit from a capacity-bounded byte array.
 *
 * @details Index zero is the least-significant bit of bytes[0]. NULL or an index outside
 * the supplied capacity produces zero without memory access. Invalid and unset are
 * intentionally indistinguishable; validate separately if that distinction is needed.
 * @param[in] bits Readable byte array, or NULL.
 * @param[in] capacity Actual readable allocation size in bytes, not bits.
 * @param[in] index Zero-based bit index across the entire array.
 * @return One if the valid selected bit is set; zero if unset or invalid.
 * @par Usage
 * emb_bitset_test(seen, sizeof(seen), channel_id) checks whether a channel was seen.
 */
static inline uint8_t emb_bitset_test(const uint8_t *bits, size_t capacity, size_t index)
{
    uint8_t result = 0;
    size_t byte_index = index / 8u;
    if ((bits != NULL) && (byte_index < capacity))
    {
        result = emb_u8_has_any(bits[byte_index], emb_u8_bit((uint8_t)(index % 8u)));
    }
    return result;
}

/**
 * @brief Set one bit in a capacity-bounded byte array.
 *
 * @details Other bits are preserved. Setting an already-set bit succeeds. Invalid input leaves
 * storage unchanged. This read/modify/write operation is not atomic across tasks/ISRs.
 * @param[in,out] bits Writable byte array, or NULL.
 * @param[in] capacity Actual writable allocation size in bytes.
 * @param[in] index Zero-based bit index, with LSB-first order inside each byte.
 * @return One if the index was valid and set; zero for NULL/out-of-range input.
 * @par Usage
 * Use a zero-initialized byte array to maintain a compact channel bitmap.
 */
static inline uint8_t emb_bitset_set(uint8_t *bits, size_t capacity, size_t index)
{
    uint8_t result = 0;
    size_t byte_index = index / 8u;
    if ((bits != NULL) && (byte_index < capacity))
    {
        bits[byte_index] = EMB_U8_OR(bits[byte_index], emb_u8_bit((uint8_t)(index % 8u)));
        result = 1;
    }
    return result;
}

/**
 * @brief Clear one bit in a capacity-bounded byte array.
 *
 * @details Other bits are preserved. Clearing an unset bit succeeds. Invalid input leaves
 * storage unchanged. This read/modify/write operation is not atomic across tasks/ISRs.
 * @param[in,out] bits Writable byte array, or NULL.
 * @param[in] capacity Actual writable allocation size in bytes.
 * @param[in] index Zero-based bit index, with LSB-first order inside each byte.
 * @return One if the index was valid and cleared; zero for NULL/out-of-range input.
 * @par Usage
 * emb_bitset_clear(pending, sizeof(pending), index) after handling one pending entry.
 */
static inline uint8_t emb_bitset_clear(uint8_t *bits, size_t capacity, size_t index)
{
    uint8_t result = 0;
    size_t byte_index = index / 8u;
    if ((bits != NULL) && (byte_index < capacity))
    {
        bits[byte_index] =
            (uint8_t)(bits[byte_index] & (uint8_t)~emb_u8_bit((uint8_t)(index % 8u)));
        result = 1;
    }
    return result;
}

/** @} */
#endif /* EMBEDDED_COMMON_BITS_H */
