/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../../kymo_config.h"
#if KYMO_EFFECTIVE_PARAMETERS
#include "kymo_param.h"
#include "../../communication/kymo_control_rx.h"
#include "../../binary/embedded_bits.h"
#include "../../binary/embedded_bytes.h"
#include <math.h>

/****************************************************************************************************
 * KymoParam_textLength
 ***************************************************************************************************/
/* Bounded strlen: returns limit + 1 for longer text, zero for NULL. */
static size_t KymoParam_textLength(const char *text, size_t limit)
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

/****************************************************************************************************
 * KymoParam_entryValid
 ***************************************************************************************************/
static uint8_t KymoParam_entryValid(const KymoParameter *entry)
{
    uint8_t valid =
        (uint8_t)((entry->pTarget != nullptr) && isfinite(entry->minimum) &&
                  isfinite(entry->maximum) && (entry->minimum <= entry->maximum) &&
                  (KymoParam_textLength(entry->pName, KYMO_PARAM_NAME_MAX) <=
                   KYMO_PARAM_NAME_MAX) &&
                  (KymoParam_textLength(entry->pUnit, KYMO_PARAM_UNIT_MAX) <= KYMO_PARAM_UNIT_MAX));
    return valid;
}

/****************************************************************************************************
 * KymoParam_idsUnique
 ***************************************************************************************************/
static uint8_t KymoParam_idsUnique(const KymoParameter *table, size_t count)
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

/****************************************************************************************************
 * KymoParam_init
 ***************************************************************************************************/
KymoStatus KymoParam_init(KymoParamReceiver *receiver, const KymoParameter *table, size_t count)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    uint8_t valid = (uint8_t)((receiver != nullptr) && (table != nullptr) && (count >= 1u) &&
                              (count <= KYMO_MAX_PARAMETERS));
    for (size_t index = 0u; (index < count) && valid; ++index)
    {
        valid = KymoParam_entryValid(&table[index]);
    }
    if (valid && KymoParam_idsUnique(table, count))
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

/****************************************************************************************************
 * KymoParam_findIndex
 ***************************************************************************************************/
/* Returns count when the ID is not registered. */
static size_t KymoParam_findIndex(const KymoParamReceiver *receiver, uint8_t id)
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

/****************************************************************************************************
 * KymoParam_applyRequest
 ***************************************************************************************************/
/* LIST queues every description and a readback of every current value. Operations
 * of the other back-channel modules are ignored; unknown ones count as invalid. */
static void KymoParam_applyRequest(KymoParamReceiver *receiver, const uint8_t *frame, size_t length)
{
    if (!KymoControlRx_crcValid(frame, length))
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
    else if ((frame[4] == KYMO_PARAM_OP_LIST) || !KymoControlRx_isKnownOperation(frame[4]))
    {
        ++receiver->invalid;
    }
    else
    {
        /* Another module's request. */
    }
}

/****************************************************************************************************
 * KymoParam_applyWrite
 ***************************************************************************************************/
/* One validated write: registered ID and a value within the limits. */
static size_t KymoParam_applyWrite(KymoParamReceiver *receiver, uint8_t id, float value)
{
    size_t applied = 0u;
    const size_t index = KymoParam_findIndex(receiver, id);
    if (index == receiver->count)
    {
        ++receiver->rejected;
    }
    else
    {
        const KymoParameter *entry = &receiver->table[index];
        if ((value >= entry->minimum) && (value <= entry->maximum))
        {
            *entry->pTarget = value;
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

/****************************************************************************************************
 * KymoParam_applyFrame
 ***************************************************************************************************/
/* A write must be CRC-protected and carry only an ID and a value. */
static size_t KymoParam_applyFrame(KymoParamReceiver *receiver, const uint8_t *frame, size_t length)
{
    size_t applied = 0u;
    KymoDataPoint point = {};
    if (frame[2] == KYMO_DESCRIPTOR_PARAM)
    {
        KymoParam_applyRequest(receiver, frame, length);
    }
    else if (!emb_u8_has_any(frame[2], KYMO_DESCRIPTOR_NO_CRC) &&
             (KymoProtocol_decode(frame, length, &point) == KYMO_OK) && (point.flags == 0u))
    {
        applied = KymoParam_applyWrite(receiver, point.id, point.value);
    }
    else
    {
        ++receiver->invalid;
    }
    return applied;
}

/****************************************************************************************************
 * KymoParam_feed
 ***************************************************************************************************/
size_t KymoParam_feed(KymoParamReceiver *receiver, const uint8_t *bytes, size_t length)
{
    size_t applied = 0u;
    if ((receiver != nullptr) && (receiver->table != nullptr) && (bytes != nullptr))
    {
        for (size_t offset = 0u; offset < length; ++offset)
        {
            const size_t complete =
                KymoControlRx_byte(&receiver->rx, bytes[offset], &receiver->invalid);
            if (complete != 0u)
            {
                applied += KymoParam_applyFrame(receiver, receiver->rx.frame, complete);
            }
        }
    }
    return applied;
}

/****************************************************************************************************
 * KymoParam_feedCompact
 ***************************************************************************************************/
size_t KymoParam_feedCompact(KymoParamReceiver *receiver, const uint8_t *data, size_t length)
{
    size_t applied = 0u;
    if ((receiver != nullptr) && (receiver->table != nullptr) && (data != nullptr))
    {
        const float value = (length == KYMO_PARAM_COMPACT_SIZE) ? emb_read_f32_le(data + 1) : 0.0f;
        if ((length == KYMO_PARAM_COMPACT_SIZE) && isfinite(value))
        {
            applied = KymoParam_applyWrite(receiver, data[0], value);
        }
        else
        {
            ++receiver->invalid;
        }
    }
    return applied;
}

/****************************************************************************************************
 * KymoParam_nextReadback
 ***************************************************************************************************/
KymoStatus KymoParam_nextReadback(KymoParamReceiver *receiver, KymoDataPoint *point)
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
                point->value = *receiver->table[index].pTarget;
                receiver->cursor = (index + 1u) % receiver->count;
                result = KYMO_OK;
            }
        }
    }
    return result;
}

/****************************************************************************************************
 * KymoParam_writeText
 ***************************************************************************************************/
/* Length byte plus text; text was bounded by KymoParam_init. */
static size_t KymoParam_writeText(uint8_t *output, const char *text, size_t limit)
{
    const size_t length = KymoParam_textLength(text, limit);
    output[0] = (uint8_t)length;
    for (size_t index = 0u; index < length; ++index)
    {
        output[1u + index] = (uint8_t)text[index];
    }
    return length + 1u;
}

/****************************************************************************************************
 * KymoParam_nextDescription
 ***************************************************************************************************/
size_t KymoParam_nextDescription(KymoParamReceiver *receiver, uint8_t *output, size_t capacity)
{
    size_t length = 0u;
    if ((receiver != nullptr) && (output != nullptr) && (receiver->table != nullptr) &&
        (receiver->describe_next < receiver->count))
    {
        const size_t index = receiver->describe_next;
        const KymoParameter *entry = &receiver->table[index];
        const size_t needed = 19u + KymoParam_textLength(entry->pName, KYMO_PARAM_NAME_MAX) +
                              KymoParam_textLength(entry->pUnit, KYMO_PARAM_UNIT_MAX);
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
            offset += KymoParam_writeText(output + offset, entry->pName, KYMO_PARAM_NAME_MAX);
            offset += KymoParam_writeText(output + offset, entry->pUnit, KYMO_PARAM_UNIT_MAX);
            output[3] = (uint8_t)(offset - 4u);
            output[offset] = KymoProtocol_crc8(output + 2, offset - 2u);
            length = offset + 1u;
            receiver->describe_next = index + 1u;
        }
    }
    return length;
}

#endif /* KYMO_EFFECTIVE_PARAMETERS */
