/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/* Known-answer tests for the header-only common utilities (C99). */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "common/embedded_bits.h"
#include "common/embedded_bytes.h"
#include "common/embedded_crc.h"

/**
 * main
 */
int main(void)
{
    static const uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint8_t raw[8] = {0};
    uint8_t bits[4] = {0};
    int result = 0;

    assert(emb_crc8_msb(check, sizeof(check), 0x07u, 0u) == 0xF4u);
    assert(emb_crc16_ccitt(check, sizeof(check)) == 0x29B1u);
    assert(emb_crc32_ieee(check, sizeof(check)) == UINT32_C(0xCBF43926));
    assert(emb_crc16_ccitt(NULL, 5u) == 0xFFFFu);
    assert(emb_crc32_ieee(NULL, 5u) == 0u);

    emb_write_u16_le(raw, 0x1234u);
    assert((raw[0] == 0x34u) && (raw[1] == 0x12u));
    emb_write_u32_le(raw, UINT32_C(0x12345678));
    assert(emb_read_u32_le(raw) == UINT32_C(0x12345678));
    assert(raw[0] == 0x78u);
    emb_write_u64_le(raw, UINT64_C(0x0102030405060708));
    assert((raw[0] == 0x08u) && (raw[7] == 0x01u));
    emb_write_f32_le(raw, 1.0f);
    assert((raw[2] == 0x80u) && (raw[3] == 0x3Fu));
    assert(emb_read_f32_le(raw) == 1.0f);

    assert(EMB_U8_OR(0x01u, 0x80u) == 0x81u);
    assert(emb_u8_only_bits(0x05u, 0x07u) == 1u);
    assert(emb_u8_only_bits(0x09u, 0x07u) == 0u);
    assert(emb_u8_bit(8u) == 0u);
    assert(emb_bitset_set(bits, sizeof(bits), 31u) == 1u);
    assert(emb_bitset_test(bits, sizeof(bits), 31u) == 1u);
    assert(emb_bitset_set(bits, sizeof(bits), 32u) == 0u);
    assert(memcmp(bits, "\0\0\0\x80", 4u) == 0);
    assert(emb_bitset_set(bits, sizeof(bits), 30u) == 1u);
    assert(emb_bitset_clear(bits, sizeof(bits), 31u) == 1u);
    assert(emb_bitset_test(bits, sizeof(bits), 31u) == 0u);
    assert(emb_bitset_test(bits, sizeof(bits), 30u) == 1u);
    assert(emb_bitset_clear(bits, sizeof(bits), 32u) == 0u);
    assert(emb_bitset_clear(NULL, sizeof(bits), 0u) == 0u);
    assert(memcmp(bits, "\0\0\0\x40", 4u) == 0);

    puts("Common utilities passed");
    return result;
}
