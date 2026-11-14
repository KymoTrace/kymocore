/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_endpoint_plot.cpp
 * @brief Plot runtime binding to the common sender.
 */
#include "kymo_core_endpoint_plot.h"
#include "../../core/detail/kymo_core_endpoint_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_RUNTIME

/****************************************************************************************************
 * KymoCoreEndpointPlot_copyPlot
 ***************************************************************************************************/
static uint8_t KymoCoreEndpointPlot_copyPlot(void *user, const uint8_t *bytes, uint8_t length)
{
    return (uint8_t)KymoCoreEndpoint_copy(user, bytes, length);
}

/****************************************************************************************************
 * KymoCoreEndpointPlot_producePlot
 ***************************************************************************************************/
static KymoStatus KymoCoreEndpointPlot_producePlot(void *user, uint8_t source, uint32_t now,
                                                   uint8_t draining)
{
    KymoCoreEndpointPlot *feature = static_cast<KymoCoreEndpointPlot *>(user);
    KymoStatus status = KYMO_IDLE;
    (void)source;
    (void)now;
    if (draining == 0u)
    {
        status = KymoRuntime_main(&feature->runtime);
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpointPlot_init
 ***************************************************************************************************/
KymoStatus KymoCoreEndpointPlot_init(KymoCoreEndpoint *endpoint, KymoCoreEndpointPlot *feature,
                                     const KymoCorePlotConfig *config)
{
    KymoCoreEndpointFeature *binding = feature != nullptr ? &feature->binding : nullptr;
    KymoStatus status = KymoCoreEndpoint_prepare(endpoint, binding, KYMO_ENDPOINT_PLOT,
                                                 KYMO_FRAME_MAX_SIZE, 1u, 0u);
    if (status == KYMO_OK)
    {
        status = KYMO_BAD_CONFIG;
        if (config != nullptr)
        {
            KymoConfig runtime = {};
            runtime.channels = config->pChannelsAv;
            runtime.channel_count = config->channelCount;
            runtime.sample = config->sampleCbk;
            runtime.sample_user = config->pSampleContext;
            runtime.clock_ms = endpoint->config.clock_ms;
            runtime.clock_user = endpoint->config.clock_user;
            runtime.write = KymoCoreEndpointPlot_copyPlot;
            runtime.transport_user = endpoint;
            status = KymoRuntime_init(&feature->runtime, &runtime);
#if KYMO_EFFECTIVE_TIMESTAMP

            /* Every Endpoint DATA source uses the same device clock origin. */
            feature->runtime.start_ms = 0u;
#endif
            for (uint16_t index = 0u; (status == KYMO_OK) && (index < config->channelCount);
                 ++index)
            {
                status = KymoCoreEndpoint_reserve(endpoint, config->pChannelsAv[index].id);
            }
        }
        binding->user = feature;
        binding->sources = 1u;
        binding->produce = KymoCoreEndpointPlot_producePlot;
    }
    return KymoCoreEndpoint_finish(endpoint, binding, status);
}
#endif
