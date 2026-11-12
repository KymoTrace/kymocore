/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @brief Table-free CRC8, CRC-16/CCITT-FALSE and CRC-32 (IEEE) helpers.
 *
 * @details Memory consumption is constant and work is bounded by eight bit
 * iterations per input byte. CRC8 is configurable and applies no reflection or
 * final XOR; the 16- and 32-bit variants implement their fixed standard parameters.
 * @file embedded_crc.h
 * @defgroup embedded_crc Common checksum
 * @{
 * @par Usage
 * Use polynomial 0x07 and initial zero for Kymo CRC-8/ATM; CRC-16 and CRC-32
 * protect the trace envelope and administrative records.
 */
#ifndef EMBEDDED_COMMON_CRC_H
/**
 * @brief Include guard for embedded_crc.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define EMBEDDED_COMMON_CRC_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Accumulate an MSB-first CRC8 over a byte range.
 *
 * @details NULL is deliberately treated as an empty input regardless of length and returns
 * initial. For a non-null pointer, the caller guarantees length readable bytes.
 * A prior result can seed the next chunk; apply any protocol-specific final XOR outside.
 * @param[in] data Readable bytes, or NULL for empty-input behavior.
 * @param[in] length Number of input bytes; zero preserves initial.
 * @param[in] polynomial Low eight polynomial bits, with the x^8 term implicit.
 * @param[in] initial Initial remainder, or a previous chunk remainder.
 * @return Final raw eight-bit remainder, without reflection/xor-out.
 * @par Usage
 * uint8_t crc = emb_crc8_msb(data, length, 0x07u, 0u);
 */
static inline uint8_t emb_crc8_msb(const uint8_t *data, size_t length, uint8_t polynomial,
                                   uint8_t initial)
{
    uint8_t result = initial;
    size_t index;
    if (data != NULL)
    {
        for (index = 0; index < length; ++index)
        {
            uint8_t bit;
            result = (uint8_t)(result ^ data[index]);
            for (bit = 0; bit < 8u; ++bit)
            {
                uint8_t high = (uint8_t)(result & 0x80u);
                result = (uint8_t)((unsigned int)result << 1u);
                if (high != 0u)
                {
                    result = (uint8_t)(result ^ polynomial);
                }
            }
        }
    }
    return result;
}

/**
 * @brief Compute CRC-16/CCITT-FALSE over a byte range.
 *
 * @details Polynomial 0x1021, initial 0xFFFF, no reflection, no final XOR.
 * NULL is treated as empty input and returns 0xFFFF.
 * @param[in] data Readable bytes, or NULL for empty-input behavior.
 * @param[in] length Number of input bytes.
 * @return CRC-16 remainder; "123456789" yields 0x29B1.
 * @par Usage
 * uint16_t crc = emb_crc16_ccitt(frame + 2u, 18u);
 */
static inline uint16_t emb_crc16_ccitt(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xffffu;
    size_t index;
    if (data != NULL)
    {
        for (index = 0; index < length; ++index)
        {
            uint8_t bit;
            crc = (uint16_t)(crc ^ (uint16_t)((uint16_t)data[index] << 8u));
            for (bit = 0; bit < 8u; ++bit)
            {
                uint16_t shifted = (uint16_t)((uint32_t)crc << 1u);
                if ((crc & 0x8000u) != 0u)
                {
                    shifted = (uint16_t)(shifted ^ 0x1021u);
                }
                crc = shifted;
            }
        }
    }
    return crc;
}

/**
 * @brief Compute reflected CRC-32 (IEEE 802.3) over a byte range.
 *
 * @details Polynomial 0xEDB88320 (reflected), initial and final XOR 0xFFFFFFFF.
 * NULL is treated as empty input and returns zero.
 * @param[in] data Readable bytes, or NULL for empty-input behavior.
 * @param[in] length Number of input bytes.
 * @return CRC-32 value; "123456789" yields 0xCBF43926.
 * @par Usage
 * uint32_t crc = emb_crc32_ieee(record, record_length);
 */
static inline uint32_t emb_crc32_ieee(const uint8_t *data, size_t length)
{
    uint32_t crc = UINT32_C(0xffffffff);
    size_t index;
    if (data != NULL)
    {
        for (index = 0; index < length; ++index)
        {
            uint8_t bit;
            crc = crc ^ (uint32_t)data[index];
            for (bit = 0; bit < 8u; ++bit)
            {
                if ((crc & 1u) != 0u)
                {
                    crc = (crc >> 1u) ^ UINT32_C(0xedb88320);
                }
                else
                {
                    crc = crc >> 1u;
                }
            }
        }
    }
    return crc ^ UINT32_C(0xffffffff);
}

/** @} */
#endif /* EMBEDDED_COMMON_CRC_H */
