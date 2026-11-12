/* SPDX-License-Identifier: MIT */
/** @file kymo_watch.cpp
 * @brief Restricted Watch reception and shared output scheduling.
 */
#include "kymo_watch.h"
#include "../detail/kymo_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_WATCH
/*******************************************************************************
 * feedWatch
 ******************************************************************************/
static void feedWatch(void *user, const uint8_t *bytes, size_t length)
{
    KymoEndpointWatch *feature = static_cast<KymoEndpointWatch *>(user);
    (void)KymoWatch_Feed(&feature->watch, bytes, length);
}
/*******************************************************************************
 * produceWatch
 ******************************************************************************/
static KymoStatus produceWatch(void *user, uint8_t source, uint32_t now, uint8_t draining)
{
    KymoEndpointWatch *feature = static_cast<KymoEndpointWatch *>(user);
    KymoStatus status = KYMO_IDLE;
    if (source == 0u)
    {
        uint8_t frame[KYMO_WATCH_FRAME_MAX_SIZE] = {};
        size_t length = KymoWatch_NextReply(&feature->watch, frame, sizeof(frame));
        if (length != 0u)
        {
            status = kymoEndpointCopy(feature->binding.endpoint, frame, length) == length
                         ? KYMO_OK
                         : feature->binding.endpoint->error;
        }
    }
    else if (draining == 0u)
    {
        KymoDataPoint point = {};
        status = KymoWatch_NextSample(&feature->watch, now, &point);
        status = kymoEndpointSample(feature->binding.endpoint, status, &point, now);
    }
    return status;
}
/*******************************************************************************
 * KymoEndpointWatch_Init
 ******************************************************************************/
KymoStatus KymoEndpointWatch_Init(KymoEndpoint *endpoint, KymoEndpointWatch *feature,
                                  const KymoEndpointWatchConfig *config)
{
    KymoEndpointFeature *binding = feature != nullptr ? &feature->binding : nullptr;
    KymoStatus status = kymoEndpointPrepare(endpoint, binding, KYMO_ENDPOINT_WATCH,
                                            KYMO_WATCH_FRAME_MAX_SIZE, 1u, 1u);
    if (status == KYMO_OK)
    {
        status = KYMO_BAD_CONFIG;
        if (config != nullptr)
        {
            status = KymoWatch_InitRestricted(&feature->watch, &config->watch,
                                              config->first_channel, config->last_channel);
            for (uint16_t channel = config->first_channel;
                 (status == KYMO_OK) && (channel <= config->last_channel); ++channel)
            {
                status = kymoEndpointReserve(endpoint, (uint8_t)channel);
            }
        }
        binding->user = feature;
        binding->sources = 2u;
        binding->produce = produceWatch;
        binding->feed = feedWatch;
    }
    return kymoEndpointFinish(endpoint, binding, status);
}
#endif
