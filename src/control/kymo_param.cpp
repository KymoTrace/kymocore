/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../kymo_config.h"
#if KYMO_EFFECTIVE_PARAMETERS
#include "kymo_param.h"
#include "kymo_control_rx.h"
#include "../common/embedded_bits.h"
#include "../common/embedded_bytes.h"
#include <math.h>

/*******************************************************************************
 * text_length
 ******************************************************************************/
/* Bounded strlen: returns limit + 1 for longer text, zero for NULL. */
static size_t text_length(const char *text, size_t limit)
{
    size_t length = 0u;
    if (text != nullptr)
    {
        while ((length <= limit) && (text[length] != '\0'))
        {
            ++length;
        }
    }
    return length;
}

/*******************************************************************************
 * entry_valid
 ******************************************************************************/
static uint8_t entry_valid(const KymoParameter *entry)
{
    uint8_t valid =
        (uint8_t)((entry->target != nullptr) && isfinite(entry->minimum) &&
                  isfinite(entry->maximum) && (entry->minimum <= entry->maximum) &&
                  (text_length(entry->name, KYMO_PARAM_NAME_MAX) <= KYMO_PARAM_NAME_MAX) &&
                  (text_length(entry->unit, KYMO_PARAM_UNIT_MAX) <= KYMO_PARAM_UNIT_MAX));
    return valid;
}

/*******************************************************************************
 * ids_unique
 ******************************************************************************/
static uint8_t ids_unique(const KymoParameter *table, size_t count)
{
    uint8_t seen[32] = {};
    uint8_t unique = 1u;
    for (size_t index = 0u; (index < count) && unique; ++index)
    {
        unique = (uint8_t)!emb_bitset_test(seen, sizeof(seen), table[index].id);
        (void)emb_bitset_set(seen, sizeof(seen), table[index].id);
    }
    return unique;
}

/*******************************************************************************
 * KymoParam_Init
 ******************************************************************************/
KymoStatus KymoParam_Init(KymoParamReceiver *receiver, const KymoParameter *table, size_t count)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    uint8_t valid = (uint8_t)((receiver != nullptr) && (table != nullptr) && (count >= 1u) &&
                              (count <= KYMO_MAX_PARAMETERS));
    for (size_t index = 0u; (index < count) && valid; ++index)
    {
        valid = entry_valid(&table[index]);
    }
    if (valid && ids_unique(table, count))
    {
        *receiver = KymoParamReceiver();
        receiver->table = table;
        receiver->count = count;
        receiver->describe_next = count;
        result = KYMO_OK;
    }
    else if (receiver != nullptr)
    {
        *receiver = KymoParamReceiver();
    }
    return result;
}

/*******************************************************************************
 * find_index
 ******************************************************************************/
/* Returns count when the ID is not registered. */
static size_t find_index(const KymoParamReceiver *receiver, uint8_t id)
{
    size_t found = receiver->count;
    for (size_t index = 0u; (index < receiver->count) && (found == receiver->count); ++index)
    {
        if (receiver->table[index].id == id)
        {
            found = index;
        }
    }
    return found;
}

/*******************************************************************************
 * apply_request
 ******************************************************************************/
/* LIST queues every description and a readback of every current value. Operations
 * of the other back-channel modules are ignored; unknown ones count as invalid. */
static void apply_request(KymoParamReceiver *receiver, const uint8_t *frame, size_t length)
{
    if (!KymoControlRx_CrcValid(frame, length))
    {
        ++receiver->invalid;
    }
    else if ((frame[4] == KYMO_PARAM_OP_LIST) && (frame[3] == 1u))
    {
        receiver->describe_next = 0u;
        for (size_t index = 0u; index < receiver->count; ++index)
        {
            (void)emb_bitset_set(receiver->pending, sizeof(receiver->pending), index);
        }
    }
    else if ((frame[4] == KYMO_PARAM_OP_LIST) || !KymoControl_IsKnownOperation(frame[4]))
    {
        ++receiver->invalid;
    }
    else
    {
        /* Another module's request. */
    }
}

/*******************************************************************************
 * apply_write
 ******************************************************************************/
/* One validated write: registered ID and a value within the limits. */
static size_t apply_write(KymoParamReceiver *receiver, uint8_t id, float value)
{
    size_t applied = 0u;
    const size_t index = find_index(receiver, id);
    if (index == receiver->count)
    {
        ++receiver->rejected;
    }
    else
    {
        const KymoParameter *entry = &receiver->table[index];
        if ((value >= entry->minimum) && (value <= entry->maximum))
        {
            *entry->target = value;
            ++receiver->accepted;
            applied = 1u;
        }
        else
        {
            ++receiver->rejected;
        }
        (void)emb_bitset_set(receiver->pending, sizeof(receiver->pending), index);
    }
    return applied;
}

/*******************************************************************************
 * apply_frame
 ******************************************************************************/
/* A write must be CRC-protected and carry only an ID and a value. */
static size_t apply_frame(KymoParamReceiver *receiver, const uint8_t *frame, size_t length)
{
    size_t applied = 0u;
    KymoDataPoint point = {};
    if (frame[2] == KYMO_DESCRIPTOR_PARAM)
    {
        apply_request(receiver, frame, length);
    }
    else if (!emb_u8_has_any(frame[2], KYMO_DESCRIPTOR_NO_CRC) &&
             (KymoProtocol_Decode(frame, length, &point) == KYMO_OK) && (point.flags == 0u))
    {
        applied = apply_write(receiver, point.id, point.value);
    }
    else
    {
        ++receiver->invalid;
    }
    return applied;
}

/*******************************************************************************
 * KymoParam_Feed
 ******************************************************************************/
size_t KymoParam_Feed(KymoParamReceiver *receiver, const uint8_t *bytes, size_t length)
{
    size_t applied = 0u;
    if ((receiver != nullptr) && (receiver->table != nullptr) && (bytes != nullptr))
    {
        for (size_t offset = 0u; offset < length; ++offset)
        {
            const size_t complete =
                KymoControlRx_Byte(&receiver->rx, bytes[offset], &receiver->invalid);
            if (complete != 0u)
            {
                applied += apply_frame(receiver, receiver->rx.frame, complete);
            }
        }
    }
    return applied;
}

/*******************************************************************************
 * KymoParam_FeedCompact
 ******************************************************************************/
size_t KymoParam_FeedCompact(KymoParamReceiver *receiver, const uint8_t *data, size_t length)
{
    size_t applied = 0u;
    if ((receiver != nullptr) && (receiver->table != nullptr) && (data != nullptr))
    {
        const float value = (length == KYMO_PARAM_COMPACT_SIZE) ? emb_read_f32_le(data + 1) : 0.0f;
        if ((length == KYMO_PARAM_COMPACT_SIZE) && isfinite(value))
        {
            applied = apply_write(receiver, data[0], value);
        }
        else
        {
            ++receiver->invalid;
        }
    }
    return applied;
}

/*******************************************************************************
 * KymoParam_NextReadback
 ******************************************************************************/
KymoStatus KymoParam_NextReadback(KymoParamReceiver *receiver, KymoDataPoint *point)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((receiver != nullptr) && (point != nullptr) && (receiver->table != nullptr))
    {
        result = KYMO_IDLE;
        for (size_t step = 0u; (step < receiver->count) && (result == KYMO_IDLE); ++step)
        {
            const size_t index = (receiver->cursor + step) % receiver->count;
            if (emb_bitset_test(receiver->pending, sizeof(receiver->pending), index))
            {
                (void)emb_bitset_clear(receiver->pending, sizeof(receiver->pending), index);
                *point = KymoDataPoint();
                point->id = receiver->table[index].id;
                point->value = *receiver->table[index].target;
                receiver->cursor = (index + 1u) % receiver->count;
                result = KYMO_OK;
            }
        }
    }
    return result;
}

/*******************************************************************************
 * write_text
 ******************************************************************************/
/* Length byte plus text; text was bounded by KymoParam_Init. */
static size_t write_text(uint8_t *output, const char *text, size_t limit)
{
    const size_t length = text_length(text, limit);
    output[0] = (uint8_t)length;
    for (size_t index = 0u; index < length; ++index)
    {
        output[1u + index] = (uint8_t)text[index];
    }
    return length + 1u;
}

/*******************************************************************************
 * KymoParam_NextDescription
 ******************************************************************************/
size_t KymoParam_NextDescription(KymoParamReceiver *receiver, uint8_t *output, size_t capacity)
{
    size_t length = 0u;
    if ((receiver != nullptr) && (output != nullptr) && (receiver->table != nullptr) &&
        (receiver->describe_next < receiver->count))
    {
        const size_t index = receiver->describe_next;
        const KymoParameter *entry = &receiver->table[index];
        const size_t needed = 19u + text_length(entry->name, KYMO_PARAM_NAME_MAX) +
                              text_length(entry->unit, KYMO_PARAM_UNIT_MAX);
        if (capacity >= needed)
        {
            size_t offset = 4u;
            output[0] = KYMO_SYNC_0;
            output[1] = KYMO_SYNC_1;
            output[2] = KYMO_DESCRIPTOR_PARAM;
            output[offset++] = KYMO_PARAM_OP_DESCRIPTION;
            output[offset++] = entry->id;
            output[offset++] = (uint8_t)index;
            output[offset++] = (uint8_t)receiver->count;
            emb_write_f32_le(output + offset, entry->minimum);
            emb_write_f32_le(output + offset + 4u, entry->maximum);
            offset += 8u;
            offset += write_text(output + offset, entry->name, KYMO_PARAM_NAME_MAX);
            offset += write_text(output + offset, entry->unit, KYMO_PARAM_UNIT_MAX);
            output[3] = (uint8_t)(offset - 4u);
            output[offset] = KymoProtocol_Crc8(output + 2, offset - 2u);
            length = offset + 1u;
            receiver->describe_next = index + 1u;
        }
    }
    return length;
}

#endif /* KYMO_EFFECTIVE_PARAMETERS */
