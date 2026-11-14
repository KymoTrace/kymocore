/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../../kymo_config.h"
#if KYMO_EFFECTIVE_WATCH
#include "kymo_watch.h"
#include "../../binary/embedded_bits.h"
#include "../../binary/embedded_bytes.h"
#include <string.h>

/* Payload lengths of the host requests, including the operation byte. */
#define WATCH_INFO_LENGTH 1u
#define WATCH_SET_LENGTH 10u

/****************************************************************************************************
 * KymoWatch_rangeValid
 ***************************************************************************************************/
static uint8_t KymoWatch_rangeValid(const KymoWatchRange *range)
{
    return (uint8_t)((range->sizeBytes != 0u) &&
                     (((uint64_t)range->startAddress + range->sizeBytes) <= 0x100000000ull));
}

/****************************************************************************************************
 * KymoWatch_configValid
 ***************************************************************************************************/
static uint8_t KymoWatch_configValid(const KymoWatchConfig *config)
{
    uint8_t valid = (uint8_t)((config->ranges != nullptr) && (config->range_count >= 1u) &&
                              (config->range_count <= KYMO_WATCH_MAX_RANGES) &&
                              (config->build_id_length <= KYMO_WATCH_BUILD_ID_MAX) &&
                              ((config->build_id != nullptr) || (config->build_id_length == 0u)));
    for (size_t index = 0u; (index < config->range_count) && valid; ++index)
    {
        valid = KymoWatch_rangeValid(&config->ranges[index]);
    }
    return valid;
}

/****************************************************************************************************
 * KymoWatch_init
 ***************************************************************************************************/
KymoStatus KymoWatch_init(KymoWatch *watch, const KymoWatchConfig *config)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if (watch != nullptr)
    {
        *watch = KymoWatch();
        if ((config != nullptr) && KymoWatch_configValid(config))
        {
            watch->config = *config;
            watch->last_channel = UINT8_MAX;
            watch->ready = 1u;
            result = KYMO_OK;
        }
    }
    return result;
}

/****************************************************************************************************
 * KymoWatch_initRestricted
 ***************************************************************************************************/
KymoStatus KymoWatch_initRestricted(KymoWatch *watch, const KymoWatchConfig *config,
                                    uint8_t first_channel, uint8_t last_channel)
{
    KymoStatus status = KymoWatch_init(watch, config);
    if (status == KYMO_OK)
    {
        if (first_channel > last_channel)
        {
            watch->ready = 0u;
            status = KYMO_BAD_CONFIG;
        }
        else
        {
            watch->first_channel = first_channel;
            watch->last_channel = last_channel;
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoWatch_typeSize
 ***************************************************************************************************/
/* Bytes read for a type code; zero for unknown codes. */
static uint8_t KymoWatch_typeSize(uint8_t type)
{
    uint8_t size = 0u;
    switch (type)
    {
    case KYMO_WATCH_TYPE_U8:
    case KYMO_WATCH_TYPE_I8:
    case KYMO_WATCH_TYPE_BOOL:
        size = 1u;
        break;
    case KYMO_WATCH_TYPE_U16:
    case KYMO_WATCH_TYPE_I16:
        size = 2u;
        break;
    case KYMO_WATCH_TYPE_U32:
    case KYMO_WATCH_TYPE_I32:
    case KYMO_WATCH_TYPE_F32:
        size = 4u;
        break;
    case KYMO_WATCH_TYPE_F64:
        size = 8u;
        break;
    default:
        break;
    }
    return size;
}

/****************************************************************************************************
 * KymoWatch_insideRanges
 ***************************************************************************************************/
/* [address, address + size) lies completely inside one allowed range. */
static uint8_t KymoWatch_insideRanges(const KymoWatch *watch, uint32_t address, uint8_t size)
{
    uint8_t inside = 0u;
    const uint64_t end = (uint64_t)address + size;
    for (size_t index = 0u; (index < watch->config.range_count) && !inside; ++index)
    {
        const KymoWatchRange *range = &watch->config.ranges[index];
        inside = (uint8_t)((address >= range->startAddress) &&
                           (end <= ((uint64_t)range->startAddress + range->sizeBytes)));
    }
    return inside;
}

/****************************************************************************************************
 * KymoWatch_checkRequest
 ***************************************************************************************************/
static uint8_t KymoWatch_checkRequest(const KymoWatch *watch, uint32_t address, uint8_t type,
                                      uint16_t period_ms)
{
    uint8_t status = KYMO_WATCH_STATUS_ACCEPTED;
    const uint8_t size = KymoWatch_typeSize(type);
    if (size == 0u)
    {
        status = KYMO_WATCH_STATUS_BAD_TYPE;
    }
    else if (period_ms < KYMO_WATCH_MIN_PERIOD_MS)
    {
        status = KYMO_WATCH_STATUS_BAD_PERIOD;
    }
    else if ((address % size) != 0u)
    {
        status = KYMO_WATCH_STATUS_MISALIGNED;
    }
    else if (!KymoWatch_insideRanges(watch, address, size))
    {
        status = KYMO_WATCH_STATUS_OUT_OF_RANGE;
    }
    else
    {
        /* Accepted. */
    }
    return status;
}

/****************************************************************************************************
 * KymoWatch_stopAll
 ***************************************************************************************************/
static void KymoWatch_stopAll(KymoWatch *watch)
{
    for (size_t index = 0u; index < KYMO_MAX_WATCHES; ++index)
    {
        watch->slots[index] = KymoWatchSlot();
    }
    memset(watch->ack_pending, 0, sizeof(watch->ack_pending));
    watch->bad_slot_pending = 0u;
}

/****************************************************************************************************
 * KymoWatch_applySet
 ***************************************************************************************************/
/* Period 0 stops the slot without further checks; a refused request stops it too. */
static void KymoWatch_applySet(KymoWatch *watch, const uint8_t *payload)
{
    const uint8_t slot = payload[1];
    const uint32_t address = emb_read_u32_le(payload + 3);
    const uint8_t type = payload[7];
    const uint16_t period_ms = (uint16_t)(payload[8] | ((uint16_t)payload[9] << 8));
    if (slot >= KYMO_MAX_WATCHES)
    {
        watch->bad_slot = slot;
        watch->bad_slot_pending = 1u;
        ++watch->rejected;
    }
    else
    {
        const uint8_t status =
            (period_ms == 0u) ? KYMO_WATCH_STATUS_ACCEPTED
            : ((payload[2] < watch->first_channel) || (payload[2] > watch->last_channel))
                ? KYMO_WATCH_STATUS_OUT_OF_RANGE
                : KymoWatch_checkRequest(watch, address, type, period_ms);
        KymoWatchSlot *target = &watch->slots[slot];
        *target = KymoWatchSlot();
        if ((status == KYMO_WATCH_STATUS_ACCEPTED) && (period_ms != 0u))
        {
            target->address = address;
            target->period_ms = period_ms;
            target->channel = payload[2];
            target->type = type;
            target->fresh = 1u;
        }
        if (status == KYMO_WATCH_STATUS_ACCEPTED)
        {
            ++watch->accepted;
        }
        else
        {
            ++watch->rejected;
        }
        watch->ack_status[slot] = status;
        (void)emb_bitset_set(watch->ack_pending, sizeof(watch->ack_pending), slot);
    }
}

/****************************************************************************************************
 * KymoWatch_applyRequest
 ***************************************************************************************************/
/* Returns 1 for a handled WATCH request; other modules' messages are ignored. */
static size_t KymoWatch_applyRequest(KymoWatch *watch, const uint8_t *frame, size_t length)
{
    size_t handled = 0u;
    const uint8_t operation = frame[4];
    const uint8_t payload_length = frame[3];
    if (frame[2] != KYMO_DESCRIPTOR_PARAM)
    {
        /* A DATA frame, e.g. a parameter write. */
    }
    else if (!KymoControlRx_crcValid(frame, length))
    {
        ++watch->invalid;
    }
    else if ((operation == KYMO_WATCH_OP_INFO) && (payload_length == WATCH_INFO_LENGTH))
    {
        KymoWatch_stopAll(watch);
        watch->info_due = 1u;
        handled = 1u;
    }
    else if ((operation == KYMO_WATCH_OP_SET) && (payload_length == WATCH_SET_LENGTH))
    {
        KymoWatch_applySet(watch, frame + 4);
        handled = 1u;
    }
    else if ((operation == KYMO_WATCH_OP_INFO) || (operation == KYMO_WATCH_OP_SET) ||
             !KymoControlRx_isKnownOperation(operation))
    {
        ++watch->invalid;
    }
    else
    {
        /* Another module's request. */
    }
    return handled;
}

/****************************************************************************************************
 * KymoWatch_feed
 ***************************************************************************************************/
size_t KymoWatch_feed(KymoWatch *watch, const uint8_t *bytes, size_t length)
{
    size_t handled = 0u;
    if ((watch != nullptr) && watch->ready && (bytes != nullptr))
    {
        for (size_t offset = 0u; offset < length; ++offset)
        {
            const size_t complete = KymoControlRx_byte(&watch->rx, bytes[offset], &watch->invalid);
            if (complete != 0u)
            {
                handled += KymoWatch_applyRequest(watch, watch->rx.frame, complete);
            }
        }
    }
    return handled;
}

/****************************************************************************************************
 * KymoWatch_finishFrame
 ***************************************************************************************************/
/* Writes sync, descriptor, length and CRC around a payload at output + 4. */
static size_t KymoWatch_finishFrame(uint8_t *output, size_t payload_length)
{
    output[0] = KYMO_SYNC_0;
    output[1] = KYMO_SYNC_1;
    output[2] = KYMO_DESCRIPTOR_PARAM;
    output[3] = (uint8_t)payload_length;
    output[4u + payload_length] = KymoProtocol_crc8(output + 2, 2u + payload_length);
    return payload_length + 5u;
}

/****************************************************************************************************
 * KymoWatch_encodeInfo
 ***************************************************************************************************/
static size_t KymoWatch_encodeInfo(const KymoWatch *watch, uint8_t *output, size_t capacity)
{
    size_t length = 0u;
    const KymoWatchConfig *config = &watch->config;
    const size_t payload_length = 6u + config->build_id_length + (8u * config->range_count);
    if (capacity >= (payload_length + 5u))
    {
        size_t offset = 4u;
        output[offset++] = KYMO_WATCH_OP_INFO_REPLY;
        output[offset++] = (uint8_t)KYMO_MAX_WATCHES;
        emb_write_u16_le(output + offset, (uint16_t)KYMO_WATCH_MIN_PERIOD_MS);
        offset += 2u;
        output[offset++] = (uint8_t)config->build_id_length;
        for (size_t index = 0u; index < config->build_id_length; ++index)
        {
            output[offset++] = config->build_id[index];
        }
        output[offset++] = (uint8_t)config->range_count;
        for (size_t index = 0u; index < config->range_count; ++index)
        {
            emb_write_u32_le(output + offset, config->ranges[index].startAddress);
            emb_write_u32_le(output + offset + 4u, config->ranges[index].sizeBytes);
            offset += 8u;
        }
        length = KymoWatch_finishFrame(output, payload_length);
    }
    return length;
}

/****************************************************************************************************
 * KymoWatch_encodeAck
 ***************************************************************************************************/
static size_t KymoWatch_encodeAck(uint8_t *output, uint8_t slot, uint8_t status)
{
    output[4] = KYMO_WATCH_OP_ACK;
    output[5] = slot;
    output[6] = status;
    return KymoWatch_finishFrame(output, 3u);
}

/****************************************************************************************************
 * KymoWatch_nextAck
 ***************************************************************************************************/
/* Pending slot acknowledgements round robin, then a BAD_SLOT one. */
static size_t KymoWatch_nextAck(KymoWatch *watch, uint8_t *output)
{
    size_t length = 0u;
    for (size_t step = 0u; (step < KYMO_MAX_WATCHES) && (length == 0u); ++step)
    {
        const size_t slot = (watch->ack_cursor + step) % KYMO_MAX_WATCHES;
        if (emb_bitset_test(watch->ack_pending, sizeof(watch->ack_pending), slot))
        {
            (void)emb_bitset_clear(watch->ack_pending, sizeof(watch->ack_pending), slot);
            length = KymoWatch_encodeAck(output, (uint8_t)slot, watch->ack_status[slot]);
            watch->ack_cursor = (slot + 1u) % KYMO_MAX_WATCHES;
        }
    }
    if ((length == 0u) && watch->bad_slot_pending)
    {
        watch->bad_slot_pending = 0u;
        length = KymoWatch_encodeAck(output, watch->bad_slot, KYMO_WATCH_STATUS_BAD_SLOT);
    }
    return length;
}

/****************************************************************************************************
 * KymoWatch_nextReply
 ***************************************************************************************************/
size_t KymoWatch_nextReply(KymoWatch *watch, uint8_t *output, size_t capacity)
{
    size_t length = 0u;
    if ((watch != nullptr) && watch->ready && (output != nullptr))
    {
        if (watch->info_due)
        {
            length = KymoWatch_encodeInfo(watch, output, capacity);
            if (length != 0u)
            {
                watch->info_due = 0u;
            }
        }
        else if (capacity >= 8u)
        {
            length = KymoWatch_nextAck(watch, output);
        }
        else
        {
            /* Too small; the reply stays due. */
        }
    }
    return length;
}

/****************************************************************************************************
 * KymoWatch_readDirect
 ***************************************************************************************************/
/* One access with exactly the type's width; f64 is two 32-bit reads and may tear. */
static void KymoWatch_readDirect(uint32_t address, uint8_t size, uint8_t *output)
{
    const uintptr_t location = (uintptr_t)address;
    if (size == 1u)
    {
        const uint8_t value = *(const volatile uint8_t *)location;
        memcpy(output, &value, sizeof(value));
    }
    else if (size == 2u)
    {
        const uint16_t value = *(const volatile uint16_t *)location;
        memcpy(output, &value, sizeof(value));
    }
    else if (size == 4u)
    {
        const uint32_t value = *(const volatile uint32_t *)location;
        memcpy(output, &value, sizeof(value));
    }
    else
    {
        const uint32_t first = *(const volatile uint32_t *)location;
        const uint32_t second = *(const volatile uint32_t *)(location + 4u);
        memcpy(output, &first, sizeof(first));
        memcpy(output + 4, &second, sizeof(second));
    }
}

/****************************************************************************************************
 * KymoWatch_toFloat
 ***************************************************************************************************/
/* Converts the raw bytes, in the device's byte order, to the wire value. */
static float KymoWatch_toFloat(uint8_t type, const uint8_t *raw)
{
    float value = 0.0f;
    if (type == KYMO_WATCH_TYPE_U8)
    {
        value = (float)raw[0];
    }
    else if (type == KYMO_WATCH_TYPE_I8)
    {
        value = (float)(int8_t)raw[0];
    }
    else if (type == KYMO_WATCH_TYPE_BOOL)
    {
        value = (raw[0] != 0u) ? 1.0f : 0.0f;
    }
    else if (type == KYMO_WATCH_TYPE_U16)
    {
        uint16_t number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    }
    else if (type == KYMO_WATCH_TYPE_I16)
    {
        int16_t number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    }
    else if (type == KYMO_WATCH_TYPE_U32)
    {
        uint32_t number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    }
    else if (type == KYMO_WATCH_TYPE_I32)
    {
        int32_t number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    }
    else if (type == KYMO_WATCH_TYPE_F32)
    {
        memcpy(&value, raw, sizeof(value));
    }
    else
    {
        double number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    }
    return value;
}

/****************************************************************************************************
 * KymoWatch_slotDue
 ***************************************************************************************************/
static uint8_t KymoWatch_slotDue(const KymoWatchSlot *slot, uint32_t now_ms)
{
    return (uint8_t)((slot->type != 0u) &&
                     (slot->fresh || ((int32_t)(now_ms - slot->due_ms) >= 0)));
}

/****************************************************************************************************
 * KymoWatch_reschedule
 ***************************************************************************************************/
/* Next sample one period later; a late slot restarts from now instead of catching up. */
static void KymoWatch_reschedule(KymoWatchSlot *slot, uint32_t now_ms)
{
    if (slot->fresh)
    {
        slot->fresh = 0u;
        slot->due_ms = now_ms + slot->period_ms;
    }
    else
    {
        slot->due_ms += slot->period_ms;
        if ((int32_t)(now_ms - slot->due_ms) >= 0)
        {
            slot->due_ms = now_ms + slot->period_ms;
        }
    }
}

/****************************************************************************************************
 * KymoWatch_sampleSlot
 ***************************************************************************************************/
/* Returns 1 when the variable was read into point. */
static uint8_t KymoWatch_sampleSlot(KymoWatch *watch, KymoWatchSlot *slot, KymoDataPoint *point)
{
    uint8_t raw[8] = {};
    const uint8_t size = KymoWatch_typeSize(slot->type);
    uint8_t read = 1u;
    if (watch->config.read != nullptr)
    {
        read = watch->config.read(watch->config.read_context, slot->address, size, raw);
    }
    else
    {
        KymoWatch_readDirect(slot->address, size, raw);
    }
    if (read)
    {
        *point = KymoDataPoint();
        point->id = slot->channel;
        point->value = KymoWatch_toFloat(slot->type, raw);
    }
    else
    {
        ++watch->read_failures;
    }
    return read;
}

/****************************************************************************************************
 * KymoWatch_nextSample
 ***************************************************************************************************/
KymoStatus KymoWatch_nextSample(KymoWatch *watch, uint32_t now_ms, KymoDataPoint *point)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((watch != nullptr) && watch->ready && (point != nullptr))
    {
        const size_t start = watch->sample_cursor;
        result = KYMO_IDLE;
        for (size_t step = 0u; (step < KYMO_MAX_WATCHES) && (result == KYMO_IDLE); ++step)
        {
            const size_t index = (start + step) % KYMO_MAX_WATCHES;
            KymoWatchSlot *slot = &watch->slots[index];
            if (KymoWatch_slotDue(slot, now_ms))
            {
                KymoWatch_reschedule(slot, now_ms);
                if (KymoWatch_sampleSlot(watch, slot, point))
                {
                    result = KYMO_OK;
                }
                watch->sample_cursor = (index + 1u) % KYMO_MAX_WATCHES;
            }
        }
    }
    return result;
}

#endif /* KYMO_EFFECTIVE_WATCH */
