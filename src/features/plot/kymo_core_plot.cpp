/* SPDX-License-Identifier: MIT */
/** @file kymo_core_plot.cpp
 * @brief Feature-owned state and application-to-dispatcher configuration.
 */
#include "../../kymo_core_plot.h"
#if KYMO_EFFECTIVE_CORE && KYMO_EFFECTIVE_RUNTIME
#include "kymo_core_endpoint_plot.h"
#include "../../core/detail/kymo_core_internal.h"
static KymoCoreEndpointPlot defaultPlot = {};

/****************************************************************************************************
 * KymoCorePlot_init
 ***************************************************************************************************/
/* Map configuration and register only on successful validation. */
KymoStatus KymoCorePlot_init(const KymoCorePlotConfig *pConfig)
{
    KymoStatus status = KymoCoreEndpointPlot_init(KymoCore_getEndpoint(), &defaultPlot, pConfig);
    return status;
}

/****************************************************************************************************
 * KymoCorePlot_sampleBound
 ***************************************************************************************************/
/* Use the canonical bound-value sampler. */
uint8_t KymoCorePlot_sampleBound(void *pContext, uint8_t channelId, KymoSample *pSample)
{
    uint8_t isAvailable = KymoRuntime_sampleBound(pContext, channelId, pSample);
    return isAvailable;
}
#endif
