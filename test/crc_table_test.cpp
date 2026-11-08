/* SPDX-License-Identifier: MIT */
/* Table CRCs (KYMO_ENABLE_CRC_TABLE) equal the bitwise reference for every input tried. */
#include <assert.h>
#include <stdint.h>
#include "telemetry/kymo_protocol.h"
#include "trace/kymo_trace_protocol.h"
#include "common/embedded_crc.h"

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    static const uint8_t check[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint8_t buffer[64];
    uint32_t state = 12345u;
    assert(KymoProtocol_Crc8(check, sizeof(check)) == 0xF4u);
    assert(KymoTrace_Crc16(check, sizeof(check)) == 0x29B1u);
    assert(KymoProtocol_Crc8(nullptr, 5u) == 0u && KymoTrace_Crc16(nullptr, 5u) == 0xFFFFu);
    for (uint32_t round = 0u; round < 2000u; ++round) {
        const size_t length = (size_t)(round % sizeof(buffer));
        for (size_t index = 0u; index < length; ++index) {
            state = (state * 1103515245u) + 12345u;
            buffer[index] = (uint8_t)(state >> 16u);
        }
        assert(KymoProtocol_Crc8(buffer, length) == emb_crc8_msb(buffer, length, 0x07u, 0u));
        assert(KymoTrace_Crc16(buffer, length) == emb_crc16_ccitt(buffer, length));
    }
    return 0;
}
