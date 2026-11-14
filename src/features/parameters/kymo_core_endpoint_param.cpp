/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_endpoint_param.cpp
 * @brief Parameter reception and two bounded output sources.
 */
#include "kymo_core_endpoint_param.h"
#include "../../core/detail/kymo_core_endpoint_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_PARAMETERS

/****************************************************************************************************
 * KymoCoreEndpointParam_feedParameters
 ***************************************************************************************************/
static void KymoCoreEndpointParam_feedParameters(void *user, const uint8_t *bytes, size_t length)
{
    KymoCoreEndpointParam *feature = static_cast<KymoCoreEndpointParam *>(user);
    (void)KymoParam_feed(&feature->receiver, bytes, length);
}

/****************************************************************************************************
 * KymoCoreEndpointParam_produceParameters
 ***************************************************************************************************/
static KymoStatus KymoCoreEndpointParam_produceParameters(void *user, uint8_t source, uint32_t now,
                                                          uint8_t draining)
{
    KymoCoreEndpointParam *feature = static_cast<KymoCoreEndpointParam *>(user);
    KymoStatus status = KYMO_IDLE;
    (void)now;
    (void)draining;
    if (source == 0u)
    {
        KymoDataPoint point = {};
        status = KymoParam_nextReadback(&feature->receiver, &point);
        status = KymoCoreEndpoint_sample(feature->binding.endpoint, status, &point, now);
    }
    else
    {
        uint8_t frame[KYMO_PARAM_FRAME_MAX_SIZE] = {};
        size_t length = KymoParam_nextDescription(&feature->receiver, frame, sizeof(frame));
        if (length != 0u)
        {
            status = KymoCoreEndpoint_copy(feature->binding.endpoint, frame, length) == length
                         ? KYMO_OK
                         : feature->binding.endpoint->error;
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpointParam_init
 ***************************************************************************************************/
KymoStatus KymoCoreEndpointParam_init(KymoCoreEndpoint *endpoint, KymoCoreEndpointParam *feature,
                                      const KymoCoreParamConfig *config)
{
    KymoCoreEndpointFeature *binding = feature != nullptr ? &feature->binding : nullptr;
    KymoStatus status = KymoCoreEndpoint_prepare(endpoint, binding, KYMO_ENDPOINT_PARAM,
                                                 KYMO_PARAM_FRAME_MAX_SIZE, 0u, 1u);
    if (status == KYMO_OK)
    {
        status = KYMO_BAD_CONFIG;
        if (config != nullptr)
        {
            status =
                KymoParam_init(&feature->receiver, config->pParametersAv, config->parameterCount);
            for (size_t index = 0u; (status == KYMO_OK) && (index < config->parameterCount);
                 ++index)
            {
                status = KymoCoreEndpoint_reserve(endpoint, config->pParametersAv[index].id);
            }
        }
        binding->user = feature;
        binding->sources = 2u;
        binding->produce = KymoCoreEndpointParam_produceParameters;
        binding->feed = KymoCoreEndpointParam_feedParameters;
    }
    return KymoCoreEndpoint_finish(endpoint, binding, status);
}
#endif
