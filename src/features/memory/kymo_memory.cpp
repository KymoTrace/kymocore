/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../../kymo_config.h"
#if KYMO_EFFECTIVE_MEMORY
#include "kymo_memory.h"
#include "../../binary/embedded_bytes.h"
#include "../../binary/embedded_crc.h"

/* Upper bound of every period, as for live variables. */
#define MEMORY_PERIOD_MAX_MS 60000u

/****************************************************************************************************
 * KymoMemory_kindValid
 ***************************************************************************************************/
static uint8_t KymoMemory_kindValid(uint8_t kind)
{
    return (uint8_t)((kind == KYMO_MEMORY_STACK_PEAK) || (kind == KYMO_MEMORY_HEAP_FREE) ||
                     (kind == KYMO_MEMORY_HEAP_MIN_FREE));
}

/****************************************************************************************************
 * KymoMemory_configValid
 ***************************************************************************************************/
static uint8_t KymoMemory_configValid(const KymoCoreMemoryConfig *config)
{
    uint8_t valid = (uint8_t)((config->pEntriesAv != nullptr) && (config->entryCount >= 1u) &&
                              (config->entryCount <= KYMO_MAX_MEMORY_ENTRIES) &&
                              (config->samplePeriodMs >= 1u) &&
                              (config->samplePeriodMs <= MEMORY_PERIOD_MAX_MS) &&
                              (config->infoPeriodMs <= MEMORY_PERIOD_MAX_MS) &&
                              (config->readBytesCbk != nullptr));
    for (size_t index = 0u; valid && (index < config->entryCount); ++index)
    {
        valid = KymoMemory_kindValid(config->pEntriesAv[index].kind);
    }
    return valid;
}

/****************************************************************************************************
 * KymoMemory_init
 ***************************************************************************************************/
KymoStatus KymoMemory_init(KymoMemory *memory, const KymoCoreMemoryConfig *config)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((memory != nullptr) && (config != nullptr) && KymoMemory_configValid(config))
    {
        *memory = KymoMemory();
        memory->config = *config;
        memory->sample_next = config->entryCount;
        memory->info_next = 0u;
        memory->fresh = 1u;
        memory->ready = 1u;
        result = KYMO_OK;
    }
    return result;
}

/****************************************************************************************************
 * KymoMemory_requestInfo
 ***************************************************************************************************/
void KymoMemory_requestInfo(KymoMemory *memory)
{
    if ((memory != nullptr) && memory->ready)
    {
        memory->info_next = 0u;
    }
}

/****************************************************************************************************
 * KymoMemory_nameLength
 ***************************************************************************************************/
static size_t KymoMemory_nameLength(const char *name)
{
    size_t length = 0u;
    while ((name != nullptr) && (length < KYMO_MEMORY_NAME_MAX) && (name[length] != '\0'))
    {
        ++length;
    }
    return length;
}

/****************************************************************************************************
 * KymoMemory_encodeInfo
 ***************************************************************************************************/
static size_t KymoMemory_encodeInfo(const KymoMemoryEntry *entry, uint8_t *output)
{
    const size_t name = KymoMemory_nameLength(entry->pName);
    const size_t payload_length = 7u + name;
    output[0] = KYMO_SYNC_0;
    output[1] = KYMO_SYNC_1;
    output[2] = KYMO_DESCRIPTOR_PARAM;
    output[3] = (uint8_t)payload_length;
    output[4] = KYMO_MEMORY_OP_INFO;
    output[5] = entry->channel;
    output[6] = entry->kind;
    emb_write_u32_le(output + 7, entry->totalSizeBytes);
    for (size_t index = 0u; index < name; ++index)
    {
        output[11u + index] = (uint8_t)entry->pName[index];
    }

    /* CRC-8/ATM like every control frame; available without KYMO_ENABLE_CRC. */
    output[4u + payload_length] = emb_crc8_msb(output + 2, 2u + payload_length, 0x07u, 0u);
    return payload_length + 5u;
}

/****************************************************************************************************
 * KymoMemory_startInfoRound
 ***************************************************************************************************/
/* A repeated round starts when the previous one has finished and its period has passed. */
static void KymoMemory_startInfoRound(KymoMemory *memory, uint32_t now_ms)
{
    if ((memory->info_next >= memory->config.entryCount) && (memory->config.infoPeriodMs != 0u) &&
        ((int32_t)(now_ms - memory->info_due_ms) >= 0))
    {
        memory->info_next = 0u;
    }
}

/****************************************************************************************************
 * KymoMemory_nextInfo
 ***************************************************************************************************/
size_t KymoMemory_nextInfo(KymoMemory *memory, uint32_t now_ms, uint8_t *output, size_t capacity)
{
    size_t length = 0u;
    if ((memory != nullptr) && memory->ready && (output != nullptr) &&
        (capacity >= KYMO_MEMORY_FRAME_MAX_SIZE))
    {
        KymoMemory_startInfoRound(memory, now_ms);
        if (memory->info_next < memory->config.entryCount)
        {
            length = KymoMemory_encodeInfo(&memory->config.pEntriesAv[memory->info_next], output);
            ++memory->info_next;
            if (memory->info_next >= memory->config.entryCount)
            {
                memory->info_due_ms = now_ms + memory->config.infoPeriodMs;
            }
        }
    }
    return length;
}

/****************************************************************************************************
 * KymoMemory_startSampleRound
 ***************************************************************************************************/
/* Every entry once per period; a late round restarts from now instead of catching up. */
static void KymoMemory_startSampleRound(KymoMemory *memory, uint32_t now_ms)
{
    if ((memory->sample_next >= memory->config.entryCount) &&
        (memory->fresh || ((int32_t)(now_ms - memory->sample_due_ms) >= 0)))
    {
        if (memory->fresh)
        {
            memory->fresh = 0u;
            memory->sample_due_ms = now_ms + memory->config.samplePeriodMs;
        }
        else
        {
            memory->sample_due_ms += memory->config.samplePeriodMs;
            if ((int32_t)(now_ms - memory->sample_due_ms) >= 0)
            {
                memory->sample_due_ms = now_ms + memory->config.samplePeriodMs;
            }
        }
        memory->sample_next = 0u;
    }
}

/****************************************************************************************************
 * KymoMemory_nextSample
 ***************************************************************************************************/
KymoStatus KymoMemory_nextSample(KymoMemory *memory, uint32_t now_ms, KymoDataPoint *point)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((memory != nullptr) && memory->ready && (point != nullptr))
    {
        result = KYMO_IDLE;
        KymoMemory_startSampleRound(memory, now_ms);
        while ((result == KYMO_IDLE) && (memory->sample_next < memory->config.entryCount))
        {
            const size_t index = memory->sample_next;
            uint32_t bytes = 0u;
            ++memory->sample_next;
            if (memory->config.readBytesCbk(memory->config.pReadContext, index, &bytes))
            {
                *point = KymoDataPoint();
                point->id = memory->config.pEntriesAv[index].channel;
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
