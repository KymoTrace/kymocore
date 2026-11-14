/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_memory.cpp
 * @brief Memory-specific bindings and complete initialization.
 */
#include "kymo_memory.h"
#include "../detail/kymo_internal.h"
static KymoCoreEndpointMemory sFeature;
static const uint8_t MEMORY_CHANNEL = 220u;
static const uint32_t DEMO_MEMORY_TOTAL_BYTES = 4096u;
static const uint32_t DEMO_MEMORY_FREE_BYTES = 2048u;
static const size_t DEMO_MEMORY_ENTRY = 0u;
static const uint16_t MEMORY_SAMPLE_PERIOD_MS = 200u;
static const uint16_t MEMORY_INFO_PERIOD_MS = 1000u;
static const KymoMemoryEntry ENTRIES[] = {
    {"demo free", DEMO_MEMORY_TOTAL_BYTES, KYMO_MEMORY_HEAP_FREE, MEMORY_CHANNEL}};

/****************************************************************************************************
 * readMemory
 ***************************************************************************************************/
static uint8_t readMemory(void *user, size_t index, uint32_t *bytes)
{
    uint8_t result = 0u;
    (void)user;
    if ((bytes != nullptr) && (index == DEMO_MEMORY_ENTRY))
    {
        *bytes = DEMO_MEMORY_FREE_BYTES;
        result = 1u;
    }
    return result;
}

/** @brief Fixed memory resources, callbacks and feature settings. */
static const KymoCoreMemoryConfig sMemoryConfig = {
    readMemory,           nullptr, ENTRIES, KYMO_COUNT_OF(ENTRIES), MEMORY_SAMPLE_PERIOD_MS,
    MEMORY_INFO_PERIOD_MS};

/****************************************************************************************************
 * exampleMemoryConfig
 ***************************************************************************************************/
const KymoCoreMemoryConfig &exampleMemoryConfig(void)
{
    return sMemoryConfig;
}

/****************************************************************************************************
 * KymoApp_MemoryInit
 ***************************************************************************************************/
KymoStatus KymoApp_MemoryInit(const KymoCoreMemoryConfig *config)
{
    return KymoCoreEndpointMemory_init(exampleEndpoint(), &sFeature, config);
}
