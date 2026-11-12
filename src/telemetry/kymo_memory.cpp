/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../kymo_config.h"
#if KYMO_EFFECTIVE_MEMORY
#include "kymo_memory.h"
#include "../common/embedded_bytes.h"
#include "../common/embedded_crc.h"

/* Upper bound of every period, as for live variables. */
#define MEMORY_PERIOD_MAX_MS 60000u

/*******************************************************************************
 * kind_valid
 ******************************************************************************/
static uint8_t kind_valid(uint8_t kind)
{
    return (uint8_t)((kind == KYMO_MEMORY_STACK_PEAK) || (kind == KYMO_MEMORY_HEAP_FREE) ||
                     (kind == KYMO_MEMORY_HEAP_MIN_FREE));
}

/*******************************************************************************
 * config_valid
 ******************************************************************************/
static uint8_t config_valid(const KymoMemoryConfig *config)
{
    uint8_t valid =
        (uint8_t)((config->entries != nullptr) && (config->count >= 1u) &&
                  (config->count <= KYMO_MAX_MEMORY_ENTRIES) && (config->period_ms >= 1u) &&
                  (config->period_ms <= MEMORY_PERIOD_MAX_MS) &&
                  (config->info_period_ms <= MEMORY_PERIOD_MAX_MS) && (config->read != nullptr));
    for (size_t index = 0u; valid && (index < config->count); ++index)
    {
        valid = kind_valid(config->entries[index].kind);
    }
    return valid;
}

/*******************************************************************************
 * KymoMemory_Init
 ******************************************************************************/
KymoStatus KymoMemory_Init(KymoMemory *memory, const KymoMemoryConfig *config)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((memory != nullptr) && (config != nullptr) && config_valid(config))
    {
        *memory = KymoMemory();
        memory->config = *config;
        memory->sample_next = config->count;
        memory->info_next = 0u;
        memory->fresh = 1u;
        memory->ready = 1u;
        result = KYMO_OK;
    }
    return result;
}

/*******************************************************************************
 * KymoMemory_RequestInfo
 ******************************************************************************/
void KymoMemory_RequestInfo(KymoMemory *memory)
{
    if ((memory != nullptr) && memory->ready)
    {
        memory->info_next = 0u;
    }
}

/*******************************************************************************
 * name_length
 ******************************************************************************/
static size_t name_length(const char *name)
{
    size_t length = 0u;
    while ((name != nullptr) && (length < KYMO_MEMORY_NAME_MAX) && (name[length] != '\0'))
    {
        ++length;
    }
    return length;
}

/*******************************************************************************
 * encode_info
 ******************************************************************************/
static size_t encode_info(const KymoMemoryEntry *entry, uint8_t *output)
{
    const size_t name = name_length(entry->name);
    const size_t payload_length = 7u + name;
    output[0] = KYMO_SYNC_0;
    output[1] = KYMO_SYNC_1;
    output[2] = KYMO_DESCRIPTOR_PARAM;
    output[3] = (uint8_t)payload_length;
    output[4] = KYMO_MEMORY_OP_INFO;
    output[5] = entry->channel;
    output[6] = entry->kind;
    emb_write_u32_le(output + 7, entry->total_bytes);
    for (size_t index = 0u; index < name; ++index)
    {
        output[11u + index] = (uint8_t)entry->name[index];
    }
    /* CRC-8/ATM like every control frame; available without KYMO_ENABLE_CRC. */
    output[4u + payload_length] = emb_crc8_msb(output + 2, 2u + payload_length, 0x07u, 0u);
    return payload_length + 5u;
}

/*******************************************************************************
 * start_info_round
 ******************************************************************************/
/* A repeated round starts when the previous one has finished and its period has passed. */
static void start_info_round(KymoMemory *memory, uint32_t now_ms)
{
    if ((memory->info_next >= memory->config.count) && (memory->config.info_period_ms != 0u) &&
        ((int32_t)(now_ms - memory->info_due_ms) >= 0))
    {
        memory->info_next = 0u;
    }
}

/*******************************************************************************
 * KymoMemory_NextInfo
 ******************************************************************************/
size_t KymoMemory_NextInfo(KymoMemory *memory, uint32_t now_ms, uint8_t *output, size_t capacity)
{
    size_t length = 0u;
    if ((memory != nullptr) && memory->ready && (output != nullptr) &&
        (capacity >= KYMO_MEMORY_FRAME_MAX_SIZE))
    {
        start_info_round(memory, now_ms);
        if (memory->info_next < memory->config.count)
        {
            length = encode_info(&memory->config.entries[memory->info_next], output);
            ++memory->info_next;
            if (memory->info_next >= memory->config.count)
            {
                memory->info_due_ms = now_ms + memory->config.info_period_ms;
            }
        }
    }
    return length;
}

/*******************************************************************************
 * start_sample_round
 ******************************************************************************/
/* Every entry once per period; a late round restarts from now instead of catching up. */
static void start_sample_round(KymoMemory *memory, uint32_t now_ms)
{
    if ((memory->sample_next >= memory->config.count) &&
        (memory->fresh || ((int32_t)(now_ms - memory->sample_due_ms) >= 0)))
    {
        if (memory->fresh)
        {
            memory->fresh = 0u;
            memory->sample_due_ms = now_ms + memory->config.period_ms;
        }
        else
        {
            memory->sample_due_ms += memory->config.period_ms;
            if ((int32_t)(now_ms - memory->sample_due_ms) >= 0)
            {
                memory->sample_due_ms = now_ms + memory->config.period_ms;
            }
        }
        memory->sample_next = 0u;
    }
}

/*******************************************************************************
 * KymoMemory_NextSample
 ******************************************************************************/
KymoStatus KymoMemory_NextSample(KymoMemory *memory, uint32_t now_ms, KymoDataPoint *point)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((memory != nullptr) && memory->ready && (point != nullptr))
    {
        result = KYMO_IDLE;
        start_sample_round(memory, now_ms);
        while ((result == KYMO_IDLE) && (memory->sample_next < memory->config.count))
        {
            const size_t index = memory->sample_next;
            uint32_t bytes = 0u;
            ++memory->sample_next;
            if (memory->config.read(memory->config.context, index, &bytes))
            {
                *point = KymoDataPoint();
                point->id = memory->config.entries[index].channel;
                point->value = (float)bytes;
                result = KYMO_OK;
            }
            else
            {
                ++memory->read_failures;
            }
        }
    }
    return result;
}

#endif /* KYMO_EFFECTIVE_MEMORY */
