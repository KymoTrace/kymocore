/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../kymo_config.h"
#if KYMO_ENABLED
#include "kymo_protocol.h"
#include "../common/embedded_bytes.h"
#if KYMO_ENABLE_CRC || KYMO_ENABLE_DECODER
#include "../common/embedded_crc.h"
#endif
#include <math.h>

/*******************************************************************************
 * flags_valid
 ******************************************************************************/
/* The microsecond unit is meaningless without a timestamp field. */
static uint8_t flags_valid(uint8_t flags)
{
    uint8_t valid = (uint8_t)(emb_u8_only_bits(flags, KYMO_CODEC_FLAGS) &&
        (!emb_u8_has_any(flags, KYMO_FLAG_TIMESTAMP_US) ||
         emb_u8_has_any(flags, KYMO_FLAG_TIMESTAMP)));
    return valid;
}

/*******************************************************************************
 * point_valid
 ******************************************************************************/
static uint8_t point_valid(const KymoDataPoint *point)
{
    uint8_t valid = 0;
    if (point != nullptr) {
        valid = (uint8_t)(flags_valid(point->flags) &&
            isfinite(point->value)
#if KYMO_ENABLE_X
            && (!emb_u8_has_any(point->flags, KYMO_FLAG_X) || isfinite(point->x))
#endif
#if KYMO_ENABLE_Z
            && (!emb_u8_has_any(point->flags, KYMO_FLAG_Z) || isfinite(point->z))
#endif
            );
    }
    return valid;
}

#if KYMO_ENABLE_CRC || KYMO_ENABLE_DECODER
#if KYMO_ENABLE_CRC_TABLE
/* CRC-8/ATM (polynomial 0x07) of every byte value: 256 bytes of flash, one lookup per byte. */
static const uint8_t kCrc8Table[256] = {
    0x00u, 0x07u, 0x0Eu, 0x09u, 0x1Cu, 0x1Bu, 0x12u, 0x15u, 0x38u, 0x3Fu, 0x36u, 0x31u,
    0x24u, 0x23u, 0x2Au, 0x2Du, 0x70u, 0x77u, 0x7Eu, 0x79u, 0x6Cu, 0x6Bu, 0x62u, 0x65u,
    0x48u, 0x4Fu, 0x46u, 0x41u, 0x54u, 0x53u, 0x5Au, 0x5Du, 0xE0u, 0xE7u, 0xEEu, 0xE9u,
    0xFCu, 0xFBu, 0xF2u, 0xF5u, 0xD8u, 0xDFu, 0xD6u, 0xD1u, 0xC4u, 0xC3u, 0xCAu, 0xCDu,
    0x90u, 0x97u, 0x9Eu, 0x99u, 0x8Cu, 0x8Bu, 0x82u, 0x85u, 0xA8u, 0xAFu, 0xA6u, 0xA1u,
    0xB4u, 0xB3u, 0xBAu, 0xBDu, 0xC7u, 0xC0u, 0xC9u, 0xCEu, 0xDBu, 0xDCu, 0xD5u, 0xD2u,
    0xFFu, 0xF8u, 0xF1u, 0xF6u, 0xE3u, 0xE4u, 0xEDu, 0xEAu, 0xB7u, 0xB0u, 0xB9u, 0xBEu,
    0xABu, 0xACu, 0xA5u, 0xA2u, 0x8Fu, 0x88u, 0x81u, 0x86u, 0x93u, 0x94u, 0x9Du, 0x9Au,
    0x27u, 0x20u, 0x29u, 0x2Eu, 0x3Bu, 0x3Cu, 0x35u, 0x32u, 0x1Fu, 0x18u, 0x11u, 0x16u,
    0x03u, 0x04u, 0x0Du, 0x0Au, 0x57u, 0x50u, 0x59u, 0x5Eu, 0x4Bu, 0x4Cu, 0x45u, 0x42u,
    0x6Fu, 0x68u, 0x61u, 0x66u, 0x73u, 0x74u, 0x7Du, 0x7Au, 0x89u, 0x8Eu, 0x87u, 0x80u,
    0x95u, 0x92u, 0x9Bu, 0x9Cu, 0xB1u, 0xB6u, 0xBFu, 0xB8u, 0xADu, 0xAAu, 0xA3u, 0xA4u,
    0xF9u, 0xFEu, 0xF7u, 0xF0u, 0xE5u, 0xE2u, 0xEBu, 0xECu, 0xC1u, 0xC6u, 0xCFu, 0xC8u,
    0xDDu, 0xDAu, 0xD3u, 0xD4u, 0x69u, 0x6Eu, 0x67u, 0x60u, 0x75u, 0x72u, 0x7Bu, 0x7Cu,
    0x51u, 0x56u, 0x5Fu, 0x58u, 0x4Du, 0x4Au, 0x43u, 0x44u, 0x19u, 0x1Eu, 0x17u, 0x10u,
    0x05u, 0x02u, 0x0Bu, 0x0Cu, 0x21u, 0x26u, 0x2Fu, 0x28u, 0x3Du, 0x3Au, 0x33u, 0x34u,
    0x4Eu, 0x49u, 0x40u, 0x47u, 0x52u, 0x55u, 0x5Cu, 0x5Bu, 0x76u, 0x71u, 0x78u, 0x7Fu,
    0x6Au, 0x6Du, 0x64u, 0x63u, 0x3Eu, 0x39u, 0x30u, 0x37u, 0x22u, 0x25u, 0x2Cu, 0x2Bu,
    0x06u, 0x01u, 0x08u, 0x0Fu, 0x1Au, 0x1Du, 0x14u, 0x13u, 0xAEu, 0xA9u, 0xA0u, 0xA7u,
    0xB2u, 0xB5u, 0xBCu, 0xBBu, 0x96u, 0x91u, 0x98u, 0x9Fu, 0x8Au, 0x8Du, 0x84u, 0x83u,
    0xDEu, 0xD9u, 0xD0u, 0xD7u, 0xC2u, 0xC5u, 0xCCu, 0xCBu, 0xE6u, 0xE1u, 0xE8u, 0xEFu,
    0xFAu, 0xFDu, 0xF4u, 0xF3u
};
#endif

/*******************************************************************************
 * KymoProtocol_Crc8
 ******************************************************************************/
uint8_t KymoProtocol_Crc8(const uint8_t *data, size_t length)
{
#if KYMO_ENABLE_CRC_TABLE
    uint8_t result = 0u;
    for (size_t index = 0u; (data != nullptr) && (index < length); ++index) {
        result = kCrc8Table[(uint8_t)(result ^ data[index])];
    }
#else
    uint8_t result = emb_crc8_msb(data, length, 0x07u, 0u);
#endif
    return result;
}

#endif

/*******************************************************************************
 * KymoProtocol_FrameLength
 ******************************************************************************/
size_t KymoProtocol_FrameLength(uint8_t descriptor)
{
    size_t length = 0;
    if ((emb_u8_mask(descriptor, KYMO_DESCRIPTOR_VERSION_MASK) == KYMO_DESCRIPTOR_DATA) &&
        !emb_u8_has_any(descriptor, KYMO_DESCRIPTOR_RESERVED_MASK) &&
        flags_valid(emb_u8_mask(descriptor, EMB_U8_OR(KYMO_ALLOWED_FLAGS,
                                                      KYMO_FLAG_TIMESTAMP_US)))) {
        length = KYMO_FRAME_MIN_SIZE;
#if KYMO_ENABLE_X
        if (emb_u8_has_any(descriptor, KYMO_FLAG_X)) {
            length += 4u;
        }
#endif
#if KYMO_ENABLE_Z
        if (emb_u8_has_any(descriptor, KYMO_FLAG_Z)) {
            length += 4u;
        }
#endif
#if KYMO_ENABLE_TIMESTAMP
        if (emb_u8_has_any(descriptor, KYMO_FLAG_TIMESTAMP)) {
            length += 4u;
        }
#endif
    }
    return length;
}

/*******************************************************************************
 * KymoProtocol_Encode
 ******************************************************************************/
size_t KymoProtocol_Encode(const KymoDataPoint *point,
                           uint8_t *output, size_t output_capacity)
{
    size_t result = 0;
    if ((output != nullptr) && point_valid(point)) {
        uint8_t descriptor = EMB_U8_OR(KYMO_DESCRIPTOR_DATA, point->flags);
#if !KYMO_ENABLE_CRC
        descriptor = EMB_U8_OR(descriptor, KYMO_DESCRIPTOR_NO_CRC);
#endif
        size_t required = KymoProtocol_FrameLength(descriptor);
        if ((required != 0u) && (output_capacity >= required)) {
            size_t offset = 4;
            output[0] = KYMO_SYNC_0;
            output[1] = KYMO_SYNC_1;
            output[2] = descriptor;
            output[3] = point->id;
#if KYMO_ENABLE_X
            if (emb_u8_has_any(point->flags, KYMO_FLAG_X)) {
                emb_write_f32_le(output + offset, point->x);
                offset += 4u;
            }
#endif
            emb_write_f32_le(output + offset, point->value);
            offset += 4u;
#if KYMO_ENABLE_Z
            if (emb_u8_has_any(point->flags, KYMO_FLAG_Z)) {
                emb_write_f32_le(output + offset, point->z);
                offset += 4u;
            }
#endif
#if KYMO_ENABLE_TIMESTAMP
            if (emb_u8_has_any(point->flags, KYMO_FLAG_TIMESTAMP)) {
                emb_write_u32_le(output + offset, point->timestamp_ms);
                offset += 4u;
            }
#endif
#if KYMO_ENABLE_CRC
            output[offset] = KymoProtocol_Crc8(output + 2, offset - 2u);
#else
            output[offset] = 0u;
#endif
            result = offset + 1u;
        }
    }
    return result;
}

#if KYMO_ENABLE_DECODER
/*******************************************************************************
 * frame_valid
 ******************************************************************************/
static uint8_t frame_valid(const uint8_t *frame, size_t length)
{
    uint8_t valid = 0;
    if ((frame != nullptr) && (length >= KYMO_FRAME_MIN_SIZE)) {
        size_t expected = KymoProtocol_FrameLength(frame[2]);
        if ((frame[0] == KYMO_SYNC_0) && (frame[1] == KYMO_SYNC_1) &&
            (expected != 0u) && (expected == length)) {
            valid = (uint8_t)(emb_u8_has_any(frame[2], KYMO_DESCRIPTOR_NO_CRC) ||
                (KymoProtocol_Crc8(frame + 2, length - 3u) == frame[length - 1u]));
        }
    }
    return valid;
}

/* Only called after frame_valid: the descriptor proves all payload bounds. */

/*******************************************************************************
 * decode_payload
 ******************************************************************************/
static void decode_payload(const uint8_t *frame, KymoDataPoint *point)
{
    size_t offset = 4;
    point->id = frame[3];
    point->flags = emb_u8_mask(frame[2], EMB_U8_OR(KYMO_ALLOWED_FLAGS, KYMO_FLAG_TIMESTAMP_US));
#if KYMO_ENABLE_X
    if (emb_u8_has_any(point->flags, KYMO_FLAG_X)) {
        point->x = emb_read_f32_le(frame + offset);
        offset += 4u;
    }
#endif
    point->value = emb_read_f32_le(frame + offset);
    offset += 4u;
#if KYMO_ENABLE_Z
    if (emb_u8_has_any(point->flags, KYMO_FLAG_Z)) {
        point->z = emb_read_f32_le(frame + offset);
        offset += 4u;
    }
#endif
#if KYMO_ENABLE_TIMESTAMP
    if (emb_u8_has_any(point->flags, KYMO_FLAG_TIMESTAMP)) {
        point->timestamp_ms = emb_read_u32_le(frame + offset);
    }
#endif
}

/*******************************************************************************
 * KymoProtocol_Decode
 ******************************************************************************/
KymoStatus KymoProtocol_Decode(const uint8_t *frame, size_t frame_length,
                               KymoDataPoint *output)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if (output != nullptr) {
        result = KYMO_BAD_SAMPLE;
        if (frame_valid(frame, frame_length)) {
            KymoDataPoint point = {};
            decode_payload(frame, &point);
            if (point_valid(&point)) {
                *output = point;
                result = KYMO_OK;
            }
        }
    }
    return result;
}

#endif /* KYMO_ENABLE_DECODER */

#endif /* KYMO_ENABLED */
