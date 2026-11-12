/* SPDX-License-Identifier: MIT */
/** @file kymo_param.cpp
 * @brief Parameter reception and two bounded output sources.
 */
#include "kymo_param.h"
#include "../detail/kymo_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_PARAMETERS
/*******************************************************************************
 * feedParameters
 ******************************************************************************/
static void feedParameters(void *user, const uint8_t *bytes, size_t length)
{
    KymoEndpointParam *feature = static_cast<KymoEndpointParam *>(user);
    (void)KymoParam_Feed(&feature->receiver, bytes, length);
}
/*******************************************************************************
 * produceParameters
 ******************************************************************************/
static KymoStatus produceParameters(void *user, uint8_t source, uint32_t now, uint8_t draining)
{
    KymoEndpointParam *feature = static_cast<KymoEndpointParam *>(user);
    KymoStatus status = KYMO_IDLE;
    (void)now;
    (void)draining;
    if (source == 0u)
    {
        KymoDataPoint point = {};
        status = KymoParam_NextReadback(&feature->receiver, &point);
        status = kymoEndpointSample(feature->binding.endpoint, status, &point, now);
    }
    else
    {
        uint8_t frame[KYMO_PARAM_FRAME_MAX_SIZE] = {};
        size_t length = KymoParam_NextDescription(&feature->receiver, frame, sizeof(frame));
        if (length != 0u)
        {
            status = kymoEndpointCopy(feature->binding.endpoint, frame, length) == length
                         ? KYMO_OK
                         : feature->binding.endpoint->error;
        }
    }
    return status;
}
/*******************************************************************************
 * KymoEndpointParam_Init
 ******************************************************************************/
KymoStatus KymoEndpointParam_Init(KymoEndpoint *endpoint, KymoEndpointParam *feature,
                                  const KymoEndpointParamConfig *config)
{
    KymoEndpointFeature *binding = feature != nullptr ? &feature->binding : nullptr;
    KymoStatus status = kymoEndpointPrepare(endpoint, binding, KYMO_ENDPOINT_PARAM,
                                            KYMO_PARAM_FRAME_MAX_SIZE, 0u, 1u);
    if (status == KYMO_OK)
    {
        status = KYMO_BAD_CONFIG;
        if (config != nullptr)
        {
            status = KymoParam_Init(&feature->receiver, config->table, config->count);
            for (size_t index = 0u; (status == KYMO_OK) && (index < config->count); ++index)
            {
                status = kymoEndpointReserve(endpoint, config->table[index].id);
            }
        }
        binding->user = feature;
        binding->sources = 2u;
        binding->produce = produceParameters;
        binding->feed = feedParameters;
    }
    return kymoEndpointFinish(endpoint, binding, status);
}
#endif
