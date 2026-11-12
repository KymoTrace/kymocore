/* SPDX-License-Identifier: MIT */
/** @file kymo_memory.cpp
 * @brief Memory-specific bindings and complete initialization.
 */
#include "kymo_memory.h"
#include "../detail/kymo_internal.h"
static KymoEndpointMemory sFeature;
static const uint8_t MEMORY_CHANNEL = 220u;
static const uint32_t DEMO_MEMORY_TOTAL_BYTES = 4096u;
static const uint32_t DEMO_MEMORY_FREE_BYTES = 2048u;
static const size_t DEMO_MEMORY_ENTRY = 0u;
static const uint16_t MEMORY_SAMPLE_PERIOD_MS = 200u;
static const uint16_t MEMORY_INFO_PERIOD_MS = 1000u;
static const KymoMemoryEntry ENTRIES[] = {
    {"demo free", DEMO_MEMORY_TOTAL_BYTES, KYMO_MEMORY_HEAP_FREE, MEMORY_CHANNEL}};
/*******************************************************************************
 * readMemory
 ******************************************************************************/
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
/*******************************************************************************
 * exampleMemoryConfig
 ******************************************************************************/
KymoEndpointMemoryConfig exampleMemoryConfig(void)
{
    KymoEndpointMemoryConfig config = {};
    config.entries = ENTRIES;
    config.count = KYMO_COUNT_OF(ENTRIES);
    config.period_ms = MEMORY_SAMPLE_PERIOD_MS;
    config.info_period_ms = MEMORY_INFO_PERIOD_MS;
    config.read = readMemory;
    return config;
}
/*******************************************************************************
 * KymoApp_MemoryInit
 ******************************************************************************/
KymoStatus KymoApp_MemoryInit(const KymoEndpointMemoryConfig *config)
{
    return KymoEndpointMemory_Init(exampleEndpoint(), &sFeature, config);
}
