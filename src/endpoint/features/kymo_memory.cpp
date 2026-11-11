/* SPDX-License-Identifier: MIT */
/** @file kymo_memory.cpp
 * @brief Memory description/sample sources with a finite stop sequence.
 */
#include "kymo_memory.h"
#include "../detail/kymo_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_MEMORY
/*******************************************************************************
 * stopMemory
 ******************************************************************************/
static void stopMemory(void *user)
{
    KymoEndpointMemory *feature = static_cast<KymoEndpointMemory *>(user);
    /* Finish the current description round, but never begin a periodic one. */
    feature->memory.config.info_period_ms = 0u;
}
/*******************************************************************************
 * produceMemory
 ******************************************************************************/
static KymoStatus produceMemory(void *user, uint8_t source, uint32_t now, uint8_t draining)
{
    KymoEndpointMemory *feature = static_cast<KymoEndpointMemory *>(user);
    KymoStatus status = KYMO_IDLE;
    if (source == 0u) {
        uint8_t frame[KYMO_MEMORY_FRAME_MAX_SIZE] = {};
        size_t length = KymoMemory_NextInfo(&feature->memory, now, frame, sizeof(frame));
        if (length != 0u) {
            status = kymoEndpointCopy(feature->binding.endpoint, frame, length) == length ?
                KYMO_OK : feature->binding.endpoint->error;
        }
    } else if (draining == 0u) {
        KymoDataPoint point = {};
        status = KymoMemory_NextSample(&feature->memory, now, &point);
        status = kymoEndpointSample(feature->binding.endpoint, status, &point, now);
    }
    return status;
}
/*******************************************************************************
 * KymoEndpointMemory_Init
 ******************************************************************************/
KymoStatus KymoEndpointMemory_Init(KymoEndpoint *endpoint, KymoEndpointMemory *feature,
    const KymoEndpointMemoryConfig *config)
{
    KymoEndpointFeature *binding = feature != nullptr ? &feature->binding : nullptr;
    KymoStatus status = kymoEndpointPrepare(endpoint, binding, KYMO_ENDPOINT_MEMORY,
        KYMO_MEMORY_FRAME_MAX_SIZE, 1u, 0u);
    if (status == KYMO_OK) {
        status = KymoMemory_Init(&feature->memory, config);
        for (size_t index = 0u; (status == KYMO_OK) && (index < config->count); ++index) {
            status = kymoEndpointReserve(endpoint, config->entries[index].channel);
        }
        binding->user = feature;
        binding->sources = 2u;
        binding->produce = produceMemory;
        binding->stop = stopMemory;
    }
    return kymoEndpointFinish(endpoint, binding, status);
}
#endif
