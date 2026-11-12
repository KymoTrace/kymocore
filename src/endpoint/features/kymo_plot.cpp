/* SPDX-License-Identifier: MIT */
/** @file kymo_plot.cpp
 * @brief Plot runtime binding to the common sender.
 */
#include "kymo_plot.h"
#include "../detail/kymo_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_RUNTIME
/*******************************************************************************
 * copyPlot
 ******************************************************************************/
static uint8_t copyPlot(void *user, const uint8_t *bytes, uint8_t length)
{
    return (uint8_t)kymoEndpointCopy(user, bytes, length);
}
/*******************************************************************************
 * producePlot
 ******************************************************************************/
static KymoStatus producePlot(void *user, uint8_t source, uint32_t now, uint8_t draining)
{
    KymoEndpointPlot *feature = static_cast<KymoEndpointPlot *>(user);
    KymoStatus status = KYMO_IDLE;
    (void)source;
    (void)now;
    if (draining == 0u)
    {
        status = Kymo_Main(&feature->runtime);
    }
    return status;
}
/*******************************************************************************
 * KymoEndpointPlot_Init
 ******************************************************************************/
KymoStatus KymoEndpointPlot_Init(KymoEndpoint *endpoint, KymoEndpointPlot *feature,
                                 const KymoEndpointPlotConfig *config)
{
    KymoEndpointFeature *binding = feature != nullptr ? &feature->binding : nullptr;
    KymoStatus status =
        kymoEndpointPrepare(endpoint, binding, KYMO_ENDPOINT_PLOT, KYMO_FRAME_MAX_SIZE, 1u, 0u);
    if (status == KYMO_OK)
    {
        status = KYMO_BAD_CONFIG;
        if (config != nullptr)
        {
            KymoConfig runtime = {};
            runtime.channels = config->channels;
            runtime.channel_count = config->channel_count;
            runtime.sample = config->sample;
            runtime.sample_user = config->sample_user;
            runtime.clock_ms = endpoint->config.clock_ms;
            runtime.clock_user = endpoint->config.clock_user;
            runtime.write = copyPlot;
            runtime.transport_user = endpoint;
            status = Kymo_Init(&feature->runtime, &runtime);
#if KYMO_EFFECTIVE_TIMESTAMP
            /* Every Endpoint DATA source uses the same device clock origin. */
            feature->runtime.start_ms = 0u;
#endif
            for (uint16_t index = 0u; (status == KYMO_OK) && (index < config->channel_count);
                 ++index)
            {
                status = kymoEndpointReserve(endpoint, config->channels[index].id);
            }
        }
        binding->user = feature;
        binding->sources = 1u;
        binding->produce = producePlot;
    }
    return kymoEndpointFinish(endpoint, binding, status);
}
#endif
