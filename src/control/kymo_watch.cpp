/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../kymo_config.h"
#if KYMO_EFFECTIVE_WATCH
#include "kymo_watch.h"
#include "../common/embedded_bits.h"
#include "../common/embedded_bytes.h"
#include <string.h>

/* Payload lengths of the host requests, including the operation byte. */
#define WATCH_INFO_LENGTH 1u
#define WATCH_SET_LENGTH 10u

/*******************************************************************************
 * range_valid
 ******************************************************************************/
static uint8_t range_valid(const KymoWatchRange *range)
{
    return (uint8_t)((range->length != 0u) &&
                     (((uint64_t)range->start + range->length) <= 0x100000000ull));
}

/*******************************************************************************
 * config_valid
 ******************************************************************************/
static uint8_t config_valid(const KymoWatchConfig *config)
{
    uint8_t valid = (uint8_t)((config->ranges != nullptr) && (config->range_count >= 1u) &&
                              (config->range_count <= KYMO_WATCH_MAX_RANGES) &&
                              (config->build_id_length <= KYMO_WATCH_BUILD_ID_MAX) &&
                              ((config->build_id != nullptr) || (config->build_id_length == 0u)));
    for (size_t index = 0u; (index < config->range_count) && valid; ++index) {
        valid = range_valid(&config->ranges[index]);
    }
    return valid;
}

/*******************************************************************************
 * KymoWatch_Init
 ******************************************************************************/
KymoStatus KymoWatch_Init(KymoWatch *watch, const KymoWatchConfig *config)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if (watch != nullptr) {
        *watch = KymoWatch();
        if ((config != nullptr) && config_valid(config)) {
            watch->config = *config;
            watch->last_channel = UINT8_MAX;
            watch->ready = 1u;
            result = KYMO_OK;
        }
    }
    return result;
}

/*******************************************************************************
 * KymoWatch_InitRestricted
 ******************************************************************************/
KymoStatus KymoWatch_InitRestricted(KymoWatch *watch, const KymoWatchConfig *config,
    uint8_t first_channel, uint8_t last_channel)
{
    KymoStatus status = KymoWatch_Init(watch, config);
    if (status == KYMO_OK) {
        if (first_channel > last_channel) {
            watch->ready = 0u;
            status = KYMO_BAD_CONFIG;
        } else {
            watch->first_channel = first_channel;
            watch->last_channel = last_channel;
        }
    }
    return status;
}

/*******************************************************************************
 * type_size
 ******************************************************************************/
/* Bytes read for a type code; zero for unknown codes. */
static uint8_t type_size(uint8_t type)
{
    uint8_t size = 0u;
    switch (type) {
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

/*******************************************************************************
 * inside_ranges
 ******************************************************************************/
/* [address, address + size) lies completely inside one allowed range. */
static uint8_t inside_ranges(const KymoWatch *watch, uint32_t address, uint8_t size)
{
    uint8_t inside = 0u;
    const uint64_t end = (uint64_t)address + size;
    for (size_t index = 0u; (index < watch->config.range_count) && !inside; ++index) {
        const KymoWatchRange *range = &watch->config.ranges[index];
        inside = (uint8_t)((address >= range->start) &&
                           (end <= ((uint64_t)range->start + range->length)));
    }
    return inside;
}

/*******************************************************************************
 * check_request
 ******************************************************************************/
static uint8_t check_request(const KymoWatch *watch, uint32_t address, uint8_t type,
                             uint16_t period_ms)
{
    uint8_t status = KYMO_WATCH_STATUS_ACCEPTED;
    const uint8_t size = type_size(type);
    if (size == 0u) {
        status = KYMO_WATCH_STATUS_BAD_TYPE;
    } else if (period_ms < KYMO_WATCH_MIN_PERIOD_MS) {
        status = KYMO_WATCH_STATUS_BAD_PERIOD;
    } else if ((address % size) != 0u) {
        status = KYMO_WATCH_STATUS_MISALIGNED;
    } else if (!inside_ranges(watch, address, size)) {
        status = KYMO_WATCH_STATUS_OUT_OF_RANGE;
    } else {
        /* Accepted. */
    }
    return status;
}

/*******************************************************************************
 * stop_all
 ******************************************************************************/
static void stop_all(KymoWatch *watch)
{
    for (size_t index = 0u; index < KYMO_MAX_WATCHES; ++index) {
        watch->slots[index] = KymoWatchSlot();
    }
    memset(watch->ack_pending, 0, sizeof(watch->ack_pending));
    watch->bad_slot_pending = 0u;
}

/*******************************************************************************
 * apply_set
 ******************************************************************************/
/* Period 0 stops the slot without further checks; a refused request stops it too. */
static void apply_set(KymoWatch *watch, const uint8_t *payload)
{
    const uint8_t slot = payload[1];
    const uint32_t address = emb_read_u32_le(payload + 3);
    const uint8_t type = payload[7];
    const uint16_t period_ms = (uint16_t)(payload[8] | ((uint16_t)payload[9] << 8));
    if (slot >= KYMO_MAX_WATCHES) {
        watch->bad_slot = slot;
        watch->bad_slot_pending = 1u;
        ++watch->rejected;
    } else {
        const uint8_t status = (period_ms == 0u) ? KYMO_WATCH_STATUS_ACCEPTED :
            ((payload[2] < watch->first_channel) || (payload[2] > watch->last_channel)) ?
                KYMO_WATCH_STATUS_OUT_OF_RANGE : check_request(watch, address, type, period_ms);
        KymoWatchSlot *target = &watch->slots[slot];
        *target = KymoWatchSlot();
        if ((status == KYMO_WATCH_STATUS_ACCEPTED) && (period_ms != 0u)) {
            target->address = address;
            target->period_ms = period_ms;
            target->channel = payload[2];
            target->type = type;
            target->fresh = 1u;
        }
        if (status == KYMO_WATCH_STATUS_ACCEPTED) {
            ++watch->accepted;
        } else {
            ++watch->rejected;
        }
        watch->ack_status[slot] = status;
        (void)emb_bitset_set(watch->ack_pending, sizeof(watch->ack_pending), slot);
    }
}

/*******************************************************************************
 * apply_request
 ******************************************************************************/
/* Returns 1 for a handled WATCH request; other modules' messages are ignored. */
static size_t apply_request(KymoWatch *watch, const uint8_t *frame, size_t length)
{
    size_t handled = 0u;
    const uint8_t operation = frame[4];
    const uint8_t payload_length = frame[3];
    if (frame[2] != KYMO_DESCRIPTOR_PARAM) {
        /* A DATA frame, e.g. a parameter write. */
    } else if (!KymoControlRx_CrcValid(frame, length)) {
        ++watch->invalid;
    } else if ((operation == KYMO_WATCH_OP_INFO) && (payload_length == WATCH_INFO_LENGTH)) {
        stop_all(watch);
        watch->info_due = 1u;
        handled = 1u;
    } else if ((operation == KYMO_WATCH_OP_SET) && (payload_length == WATCH_SET_LENGTH)) {
        apply_set(watch, frame + 4);
        handled = 1u;
    } else if ((operation == KYMO_WATCH_OP_INFO) || (operation == KYMO_WATCH_OP_SET) ||
               !KymoControl_IsKnownOperation(operation)) {
        ++watch->invalid;
    } else {
        /* Another module's request. */
    }
    return handled;
}

/*******************************************************************************
 * KymoWatch_Feed
 ******************************************************************************/
size_t KymoWatch_Feed(KymoWatch *watch, const uint8_t *bytes, size_t length)
{
    size_t handled = 0u;
    if ((watch != nullptr) && watch->ready && (bytes != nullptr)) {
        for (size_t offset = 0u; offset < length; ++offset) {
            const size_t complete = KymoControlRx_Byte(&watch->rx, bytes[offset], &watch->invalid);
            if (complete != 0u) {
                handled += apply_request(watch, watch->rx.frame, complete);
            }
        }
    }
    return handled;
}

/*******************************************************************************
 * finish_frame
 ******************************************************************************/
/* Writes sync, descriptor, length and CRC around a payload at output + 4. */
static size_t finish_frame(uint8_t *output, size_t payload_length)
{
    output[0] = KYMO_SYNC_0;
    output[1] = KYMO_SYNC_1;
    output[2] = KYMO_DESCRIPTOR_PARAM;
    output[3] = (uint8_t)payload_length;
    output[4u + payload_length] = KymoProtocol_Crc8(output + 2, 2u + payload_length);
    return payload_length + 5u;
}

/*******************************************************************************
 * encode_info
 ******************************************************************************/
static size_t encode_info(const KymoWatch *watch, uint8_t *output, size_t capacity)
{
    size_t length = 0u;
    const KymoWatchConfig *config = &watch->config;
    const size_t payload_length = 6u + config->build_id_length + (8u * config->range_count);
    if (capacity >= (payload_length + 5u)) {
        size_t offset = 4u;
        output[offset++] = KYMO_WATCH_OP_INFO_REPLY;
        output[offset++] = (uint8_t)KYMO_MAX_WATCHES;
        emb_write_u16_le(output + offset, (uint16_t)KYMO_WATCH_MIN_PERIOD_MS);
        offset += 2u;
        output[offset++] = (uint8_t)config->build_id_length;
        for (size_t index = 0u; index < config->build_id_length; ++index) {
            output[offset++] = config->build_id[index];
        }
        output[offset++] = (uint8_t)config->range_count;
        for (size_t index = 0u; index < config->range_count; ++index) {
            emb_write_u32_le(output + offset, config->ranges[index].start);
            emb_write_u32_le(output + offset + 4u, config->ranges[index].length);
            offset += 8u;
        }
        length = finish_frame(output, payload_length);
    }
    return length;
}

/*******************************************************************************
 * encode_ack
 ******************************************************************************/
static size_t encode_ack(uint8_t *output, uint8_t slot, uint8_t status)
{
    output[4] = KYMO_WATCH_OP_ACK;
    output[5] = slot;
    output[6] = status;
    return finish_frame(output, 3u);
}

/*******************************************************************************
 * next_ack
 ******************************************************************************/
/* Pending slot acknowledgements round robin, then a BAD_SLOT one. */
static size_t next_ack(KymoWatch *watch, uint8_t *output)
{
    size_t length = 0u;
    for (size_t step = 0u; (step < KYMO_MAX_WATCHES) && (length == 0u); ++step) {
        const size_t slot = (watch->ack_cursor + step) % KYMO_MAX_WATCHES;
        if (emb_bitset_test(watch->ack_pending, sizeof(watch->ack_pending), slot)) {
            (void)emb_bitset_clear(watch->ack_pending, sizeof(watch->ack_pending), slot);
            length = encode_ack(output, (uint8_t)slot, watch->ack_status[slot]);
            watch->ack_cursor = (slot + 1u) % KYMO_MAX_WATCHES;
        }
    }
    if ((length == 0u) && watch->bad_slot_pending) {
        watch->bad_slot_pending = 0u;
        length = encode_ack(output, watch->bad_slot, KYMO_WATCH_STATUS_BAD_SLOT);
    }
    return length;
}

/*******************************************************************************
 * KymoWatch_NextReply
 ******************************************************************************/
size_t KymoWatch_NextReply(KymoWatch *watch, uint8_t *output, size_t capacity)
{
    size_t length = 0u;
    if ((watch != nullptr) && watch->ready && (output != nullptr)) {
        if (watch->info_due) {
            length = encode_info(watch, output, capacity);
            if (length != 0u) {
                watch->info_due = 0u;
            }
        } else if (capacity >= 8u) {
            length = next_ack(watch, output);
        } else {
            /* Too small; the reply stays due. */
        }
    }
    return length;
}

/*******************************************************************************
 * read_direct
 ******************************************************************************/
/* One access with exactly the type's width; f64 is two 32-bit reads and may tear. */
static void read_direct(uint32_t address, uint8_t size, uint8_t *output)
{
    const uintptr_t location = (uintptr_t)address;
    if (size == 1u) {
        const uint8_t value = *(const volatile uint8_t *)location;
        memcpy(output, &value, sizeof(value));
    } else if (size == 2u) {
        const uint16_t value = *(const volatile uint16_t *)location;
        memcpy(output, &value, sizeof(value));
    } else if (size == 4u) {
        const uint32_t value = *(const volatile uint32_t *)location;
        memcpy(output, &value, sizeof(value));
    } else {
        const uint32_t first = *(const volatile uint32_t *)location;
        const uint32_t second = *(const volatile uint32_t *)(location + 4u);
        memcpy(output, &first, sizeof(first));
        memcpy(output + 4, &second, sizeof(second));
    }
}

/*******************************************************************************
 * to_float
 ******************************************************************************/
/* Converts the raw bytes, in the device's byte order, to the wire value. */
static float to_float(uint8_t type, const uint8_t *raw)
{
    float value = 0.0f;
    if (type == KYMO_WATCH_TYPE_U8) {
        value = (float)raw[0];
    } else if (type == KYMO_WATCH_TYPE_I8) {
        value = (float)(int8_t)raw[0];
    } else if (type == KYMO_WATCH_TYPE_BOOL) {
        value = (raw[0] != 0u) ? 1.0f : 0.0f;
    } else if (type == KYMO_WATCH_TYPE_U16) {
        uint16_t number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    } else if (type == KYMO_WATCH_TYPE_I16) {
        int16_t number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    } else if (type == KYMO_WATCH_TYPE_U32) {
        uint32_t number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    } else if (type == KYMO_WATCH_TYPE_I32) {
        int32_t number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    } else if (type == KYMO_WATCH_TYPE_F32) {
        memcpy(&value, raw, sizeof(value));
    } else {
        double number;
        memcpy(&number, raw, sizeof(number));
        value = (float)number;
    }
    return value;
}

/*******************************************************************************
 * slot_due
 ******************************************************************************/
static uint8_t slot_due(const KymoWatchSlot *slot, uint32_t now_ms)
{
    return (uint8_t)((slot->type != 0u) &&
                     (slot->fresh || ((int32_t)(now_ms - slot->due_ms) >= 0)));
}

/*******************************************************************************
 * reschedule
 ******************************************************************************/
/* Next sample one period later; a late slot restarts from now instead of catching up. */
static void reschedule(KymoWatchSlot *slot, uint32_t now_ms)
{
    if (slot->fresh) {
        slot->fresh = 0u;
        slot->due_ms = now_ms + slot->period_ms;
    } else {
        slot->due_ms += slot->period_ms;
        if ((int32_t)(now_ms - slot->due_ms) >= 0) {
            slot->due_ms = now_ms + slot->period_ms;
        }
    }
}

/*******************************************************************************
 * sample_slot
 ******************************************************************************/
/* Returns 1 when the variable was read into point. */
static uint8_t sample_slot(KymoWatch *watch, KymoWatchSlot *slot, KymoDataPoint *point)
{
    uint8_t raw[8] = {};
    const uint8_t size = type_size(slot->type);
    uint8_t read = 1u;
    if (watch->config.read != nullptr) {
        read = watch->config.read(watch->config.read_context, slot->address, size, raw);
    } else {
        read_direct(slot->address, size, raw);
    }
    if (read) {
        *point = KymoDataPoint();
        point->id = slot->channel;
        point->value = to_float(slot->type, raw);
    } else {
        ++watch->read_failures;
    }
    return read;
}

/*******************************************************************************
 * KymoWatch_NextSample
 ******************************************************************************/
KymoStatus KymoWatch_NextSample(KymoWatch *watch, uint32_t now_ms, KymoDataPoint *point)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((watch != nullptr) && watch->ready && (point != nullptr)) {
        const size_t start = watch->sample_cursor;
        result = KYMO_IDLE;
        for (size_t step = 0u; (step < KYMO_MAX_WATCHES) && (result == KYMO_IDLE); ++step) {
            const size_t index = (start + step) % KYMO_MAX_WATCHES;
            KymoWatchSlot *slot = &watch->slots[index];
            if (slot_due(slot, now_ms)) {
                reschedule(slot, now_ms);
                if (sample_slot(watch, slot, point)) {
                    result = KYMO_OK;
                }
                watch->sample_cursor = (index + 1u) % KYMO_MAX_WATCHES;
            }
        }
    }
    return result;
}

#endif /* KYMO_EFFECTIVE_WATCH */
