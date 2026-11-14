/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_endpoint_memory.cpp
 * @brief Memory description/sample sources with a finite stop sequence.
 */
#include "kymo_core_endpoint_memory.h"
#include "../../core/detail/kymo_core_endpoint_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_MEMORY

/****************************************************************************************************
 * KymoCoreEndpointMemory_stopMemory
 ***************************************************************************************************/
static void KymoCoreEndpointMemory_stopMemory(void *user)
{
    KymoCoreEndpointMemory *feature = static_cast<KymoCoreEndpointMemory *>(user);

    /* Finish the current description round, but never begin a periodic one. */
    feature->memory.config.infoPeriodMs = 0u;
}

/****************************************************************************************************
 * KymoCoreEndpointMemory_produceMemory
 ***************************************************************************************************/
static KymoStatus KymoCoreEndpointMemory_produceMemory(void *user, uint8_t source, uint32_t now,
                                                       uint8_t draining)
{
    KymoCoreEndpointMemory *feature = static_cast<KymoCoreEndpointMemory *>(user);
    KymoStatus status = KYMO_IDLE;
    if (source == 0u)
    {
        uint8_t frame[KYMO_MEMORY_FRAME_MAX_SIZE] = {};
        size_t length = KymoMemory_nextInfo(&feature->memory, now, frame, sizeof(frame));
        if (length != 0u)
        {
            status = KymoCoreEndpoint_copy(feature->binding.endpoint, frame, length) == length
                         ? KYMO_OK
                         : feature->binding.endpoint->error;
        }
    }
    else if (draining == 0u)
    {
        KymoDataPoint point = {};
        status = KymoMemory_nextSample(&feature->memory, now, &point);
        status = KymoCoreEndpoint_sample(feature->binding.endpoint, status, &point, now);
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpointMemory_init
 ***************************************************************************************************/
KymoStatus KymoCoreEndpointMemory_init(KymoCoreEndpoint *endpoint, KymoCoreEndpointMemory *feature,
                                       const KymoCoreMemoryConfig *config)
{
    KymoCoreEndpointFeature *binding = feature != nullptr ? &feature->binding : nullptr;
    KymoStatus status = KymoCoreEndpoint_prepare(endpoint, binding, KYMO_ENDPOINT_MEMORY,
                                                 KYMO_MEMORY_FRAME_MAX_SIZE, 1u, 0u);
    if (status == KYMO_OK)
    {
        status = KymoMemory_init(&feature->memory, config);
        for (size_t index = 0u; (status == KYMO_OK) && (index < config->entryCount); ++index)
        {
            status = KymoCoreEndpoint_reserve(endpoint, config->pEntriesAv[index].channel);
        }
        binding->user = feature;
        binding->sources = 2u;
        binding->produce = KymoCoreEndpointMemory_produceMemory;
        binding->stop = KymoCoreEndpointMemory_stopMemory;
    }
    return KymoCoreEndpoint_finish(endpoint, binding, status);
}
#endif
