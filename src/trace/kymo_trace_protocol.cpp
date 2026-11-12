/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file kymo_trace_protocol.cpp @brief Fieldwise trace protocol encoding. */
#include "kymo_trace_protocol.h"

#if KYMO_EFFECTIVE_TRACE
#include "../common/embedded_bytes.h"
#include "../common/embedded_crc.h"

#if KYMO_ENABLE_CRC_TABLE
/* CRC-16/CCITT-FALSE (polynomial 0x1021) of every byte value: 512 bytes of flash. */
static const uint16_t kCrc16Table[256] = {
    0x0000u, 0x1021u, 0x2042u, 0x3063u, 0x4084u, 0x50A5u, 0x60C6u, 0x70E7u, 0x8108u, 0x9129u,
    0xA14Au, 0xB16Bu, 0xC18Cu, 0xD1ADu, 0xE1CEu, 0xF1EFu, 0x1231u, 0x0210u, 0x3273u, 0x2252u,
    0x52B5u, 0x4294u, 0x72F7u, 0x62D6u, 0x9339u, 0x8318u, 0xB37Bu, 0xA35Au, 0xD3BDu, 0xC39Cu,
    0xF3FFu, 0xE3DEu, 0x2462u, 0x3443u, 0x0420u, 0x1401u, 0x64E6u, 0x74C7u, 0x44A4u, 0x5485u,
    0xA56Au, 0xB54Bu, 0x8528u, 0x9509u, 0xE5EEu, 0xF5CFu, 0xC5ACu, 0xD58Du, 0x3653u, 0x2672u,
    0x1611u, 0x0630u, 0x76D7u, 0x66F6u, 0x5695u, 0x46B4u, 0xB75Bu, 0xA77Au, 0x9719u, 0x8738u,
    0xF7DFu, 0xE7FEu, 0xD79Du, 0xC7BCu, 0x48C4u, 0x58E5u, 0x6886u, 0x78A7u, 0x0840u, 0x1861u,
    0x2802u, 0x3823u, 0xC9CCu, 0xD9EDu, 0xE98Eu, 0xF9AFu, 0x8948u, 0x9969u, 0xA90Au, 0xB92Bu,
    0x5AF5u, 0x4AD4u, 0x7AB7u, 0x6A96u, 0x1A71u, 0x0A50u, 0x3A33u, 0x2A12u, 0xDBFDu, 0xCBDCu,
    0xFBBFu, 0xEB9Eu, 0x9B79u, 0x8B58u, 0xBB3Bu, 0xAB1Au, 0x6CA6u, 0x7C87u, 0x4CE4u, 0x5CC5u,
    0x2C22u, 0x3C03u, 0x0C60u, 0x1C41u, 0xEDAEu, 0xFD8Fu, 0xCDECu, 0xDDCDu, 0xAD2Au, 0xBD0Bu,
    0x8D68u, 0x9D49u, 0x7E97u, 0x6EB6u, 0x5ED5u, 0x4EF4u, 0x3E13u, 0x2E32u, 0x1E51u, 0x0E70u,
    0xFF9Fu, 0xEFBEu, 0xDFDDu, 0xCFFCu, 0xBF1Bu, 0xAF3Au, 0x9F59u, 0x8F78u, 0x9188u, 0x81A9u,
    0xB1CAu, 0xA1EBu, 0xD10Cu, 0xC12Du, 0xF14Eu, 0xE16Fu, 0x1080u, 0x00A1u, 0x30C2u, 0x20E3u,
    0x5004u, 0x4025u, 0x7046u, 0x6067u, 0x83B9u, 0x9398u, 0xA3FBu, 0xB3DAu, 0xC33Du, 0xD31Cu,
    0xE37Fu, 0xF35Eu, 0x02B1u, 0x1290u, 0x22F3u, 0x32D2u, 0x4235u, 0x5214u, 0x6277u, 0x7256u,
    0xB5EAu, 0xA5CBu, 0x95A8u, 0x8589u, 0xF56Eu, 0xE54Fu, 0xD52Cu, 0xC50Du, 0x34E2u, 0x24C3u,
    0x14A0u, 0x0481u, 0x7466u, 0x6447u, 0x5424u, 0x4405u, 0xA7DBu, 0xB7FAu, 0x8799u, 0x97B8u,
    0xE75Fu, 0xF77Eu, 0xC71Du, 0xD73Cu, 0x26D3u, 0x36F2u, 0x0691u, 0x16B0u, 0x6657u, 0x7676u,
    0x4615u, 0x5634u, 0xD94Cu, 0xC96Du, 0xF90Eu, 0xE92Fu, 0x99C8u, 0x89E9u, 0xB98Au, 0xA9ABu,
    0x5844u, 0x4865u, 0x7806u, 0x6827u, 0x18C0u, 0x08E1u, 0x3882u, 0x28A3u, 0xCB7Du, 0xDB5Cu,
    0xEB3Fu, 0xFB1Eu, 0x8BF9u, 0x9BD8u, 0xABBBu, 0xBB9Au, 0x4A75u, 0x5A54u, 0x6A37u, 0x7A16u,
    0x0AF1u, 0x1AD0u, 0x2AB3u, 0x3A92u, 0xFD2Eu, 0xED0Fu, 0xDD6Cu, 0xCD4Du, 0xBDAAu, 0xAD8Bu,
    0x9DE8u, 0x8DC9u, 0x7C26u, 0x6C07u, 0x5C64u, 0x4C45u, 0x3CA2u, 0x2C83u, 0x1CE0u, 0x0CC1u,
    0xEF1Fu, 0xFF3Eu, 0xCF5Du, 0xDF7Cu, 0xAF9Bu, 0xBFBAu, 0x8FD9u, 0x9FF8u, 0x6E17u, 0x7E36u,
    0x4E55u, 0x5E74u, 0x2E93u, 0x3EB2u, 0x0ED1u, 0x1EF0u};
#endif

/*******************************************************************************
 * KymoTrace_Crc16
 ******************************************************************************/
uint16_t KymoTrace_Crc16(const uint8_t *data, size_t length)
{
#if KYMO_ENABLE_CRC_TABLE
    uint16_t crc = 0xFFFFu;
    for (size_t index = 0u; (data != NULL) && (index < length); ++index)
    {
        const uint8_t slot = (uint8_t)((uint8_t)(crc >> 8u) ^ data[index]);
        crc = (uint16_t)((uint16_t)(crc << 8u) ^ kCrc16Table[slot]);
    }
#else
    uint16_t crc = emb_crc16_ccitt(data, length);
#endif
    return crc;
}

/*******************************************************************************
 * KymoTrace_EncodeEnvelope
 ******************************************************************************/
size_t KymoTrace_EncodeEnvelope(const KymoTraceEnvelopeInfo *info, const uint8_t *payload,
                                size_t payload_length, uint8_t *output, size_t capacity)
{
    size_t result = 0u;
    if ((info != NULL) && (output != NULL) && ((payload != NULL) || (payload_length == 0u)) &&
        (payload_length <= KYMO_TRACE_FRAME_MAX - KYMO_TRACE_ENVELOPE_SIZE) &&
        (capacity >= payload_length + KYMO_TRACE_ENVELOPE_SIZE))
    {
        output[0] = 0xa5u;
        output[1] = 0x5au;
        output[2] = 0x80u;
        output[3] = info->payload_kind;
        emb_write_u16_le(output + 4u, static_cast<uint16_t>(payload_length));
        emb_write_u16_le(output + 6u, 0u);
        emb_write_u64_le(output + 8u, info->session_id);
        emb_write_u32_le(output + 16u, info->packet_seq);
        emb_write_u16_le(output + 20u, KymoTrace_Crc16(output + 2u, 18u));
        for (size_t index = 0u; index < payload_length; ++index)
        {
            output[22u + index] = payload[index];
        }
        emb_write_u16_le(output + 22u + payload_length,
                         KymoTrace_Crc16(output + 2u, 20u + payload_length));
        result = 24u + payload_length;
    }
    return result;
}

/*******************************************************************************
 * KymoTrace_EncodeBatch
 ******************************************************************************/
size_t KymoTrace_EncodeBatch(uint16_t domain, const KymoTraceEvent *events, size_t count,
                             uint8_t *output, size_t capacity)
{
    size_t result = 0u;
    if ((output != NULL) && ((events != NULL) || (count == 0u)) && (count <= 255u) &&
        (capacity >= 4u + count * KYMO_TRACE_EVENT_SIZE))
    {
        output[0] = 1u;
        output[1] = static_cast<uint8_t>(count);
        emb_write_u16_le(output + 2u, domain);
        for (size_t index = 0u; index < count; ++index)
        {
            const size_t offset = 4u + index * KYMO_TRACE_EVENT_SIZE;
            const KymoTraceEvent &event = events[index];
            emb_write_u64_le(output + offset, event.timestamp_ticks);
            emb_write_u32_le(output + offset + 8u, event.record_seq);
            emb_write_u32_le(output + offset + 12u, event.context_id);
            emb_write_u32_le(output + offset + 16u, event.context_seq);
            emb_write_u16_le(output + offset + 20u, event.symbol_id);
            emb_write_u16_le(output + offset + 22u, event.depth);
            output[offset + 24u] = event.event_type;
            output[offset + 25u] = event.core_id;
            emb_write_u16_le(output + offset + 26u, event.flags);
            emb_write_u32_le(output + offset + 28u, event.arg);
        }
        result = 4u + count * KYMO_TRACE_EVENT_SIZE;
    }
    return result;
}

/*******************************************************************************
 * KymoTrace_EncodeAdminFragment
 ******************************************************************************/
size_t KymoTrace_EncodeAdminFragment(uint8_t kind, uint32_t generation, uint32_t transfer,
                                     uint32_t total, uint32_t offset, const uint8_t *record,
                                     size_t length, uint8_t *output, size_t capacity)
{
    size_t result = 0u;
    if ((output != NULL) && ((record != NULL) || (length == 0u)) && (length <= capacity) &&
        (capacity >= KYMO_TRACE_ADMIN_PREFIX_SIZE + length) && (offset <= total) &&
        (length <= total - offset) && (kind != 0u))
    {
        output[0] = 1u;
        output[1] = kind;
        emb_write_u32_le(output + 2u, generation);
        emb_write_u32_le(output + 6u, transfer);
        emb_write_u32_le(output + 10u, total);
        emb_write_u32_le(output + 14u, offset);
        for (size_t index = 0u; index < length; ++index)
        {
            output[KYMO_TRACE_ADMIN_PREFIX_SIZE + index] = record[offset + index];
        }
        result = KYMO_TRACE_ADMIN_PREFIX_SIZE + length;
    }
    return result;
}

#endif /* KYMO_EFFECTIVE_TRACE */
