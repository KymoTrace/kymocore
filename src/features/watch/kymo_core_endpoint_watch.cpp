/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_endpoint_watch.cpp
 * @brief Restricted Watch reception and shared output scheduling.
 */
#include "kymo_core_endpoint_watch.h"
#include "../../core/detail/kymo_core_endpoint_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_WATCH

/****************************************************************************************************
 * KymoCoreEndpointWatch_feedWatch
 ***************************************************************************************************/
static void KymoCoreEndpointWatch_feedWatch(void *user, const uint8_t *bytes, size_t length)
{
    KymoCoreEndpointWatch *feature = static_cast<KymoCoreEndpointWatch *>(user);
    (void)KymoWatch_feed(&feature->watch, bytes, length);
}

/****************************************************************************************************
 * KymoCoreEndpointWatch_produceWatch
 ***************************************************************************************************/
static KymoStatus KymoCoreEndpointWatch_produceWatch(void *user, uint8_t source, uint32_t now,
                                                     uint8_t draining)
{
    KymoCoreEndpointWatch *feature = static_cast<KymoCoreEndpointWatch *>(user);
    KymoStatus status = KYMO_IDLE;
    if (source == 0u)
    {
        uint8_t frame[KYMO_WATCH_FRAME_MAX_SIZE] = {};
        size_t length = KymoWatch_nextReply(&feature->watch, frame, sizeof(frame));
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
        status = KymoWatch_nextSample(&feature->watch, now, &point);
        status = KymoCoreEndpoint_sample(feature->binding.endpoint, status, &point, now);
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpointWatch_init
 ***************************************************************************************************/
KymoStatus KymoCoreEndpointWatch_init(KymoCoreEndpoint *endpoint, KymoCoreEndpointWatch *feature,
                                      const KymoCoreEndpointWatchConfig *config)
{
    KymoCoreEndpointFeature *binding = feature != nullptr ? &feature->binding : nullptr;
    KymoStatus status = KymoCoreEndpoint_prepare(endpoint, binding, KYMO_ENDPOINT_WATCH,
                                                 KYMO_WATCH_FRAME_MAX_SIZE, 1u, 1u);
    if (status == KYMO_OK)
    {
        status = KYMO_BAD_CONFIG;
        if (config != nullptr)
        {
            status = KymoWatch_initRestricted(&feature->watch, &config->watch,
                                              config->first_channel, config->last_channel);
            for (uint16_t channel = config->first_channel;
                 (status == KYMO_OK) && (channel <= config->last_channel); ++channel)
            {
                status = KymoCoreEndpoint_reserve(endpoint, (uint8_t)channel);
            }
        }
        binding->user = feature;
        binding->sources = 2u;
        binding->produce = KymoCoreEndpointWatch_produceWatch;
        binding->feed = KymoCoreEndpointWatch_feedWatch;
    }
    return KymoCoreEndpoint_finish(endpoint, binding, status);
}
#endif
