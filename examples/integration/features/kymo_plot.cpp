/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_plot.cpp
 * @brief Plot-specific bindings and complete initialization.
 */
#include "kymo_plot.h"
#include "../detail/kymo_internal.h"
static KymoCoreEndpointPlot sFeature;
#include "../demo_values.h"
#include "../kymo_port.h"
static const uint8_t MEASUREMENT_CHANNEL = 0u;
static const uint32_t MEASUREMENT_PERIOD_MS = 50u;
static const uint32_t SAWTOOTH_PERIOD_MS = 1000u;
static const KymoChannel CHANNELS[] = {{MEASUREMENT_PERIOD_MS, MEASUREMENT_CHANNEL, 0u}};

/****************************************************************************************************
 * readSample
 ***************************************************************************************************/
static uint8_t readSample(void *user, uint8_t id, KymoSample *sample)
{
    uint8_t result = 0u;
    (void)user;
    if ((sample != nullptr) && (id == MEASUREMENT_CHANNEL))
    {
        sample->value = exampleGain * (float)(examplePortClockMs(nullptr) % SAWTOOTH_PERIOD_MS) /
                        (float)SAWTOOTH_PERIOD_MS;
        result = 1u;
    }
    return result;
}

/** @brief Fixed plot resources, callbacks and feature settings. */
static const KymoCorePlotConfig sPlotConfig = {readSample, nullptr, CHANNELS,
                                               KYMO_COUNT_OF(CHANNELS)};

/****************************************************************************************************
 * examplePlotConfig
 ***************************************************************************************************/
const KymoCorePlotConfig &examplePlotConfig(void)
{
    return sPlotConfig;
}

/****************************************************************************************************
 * KymoApp_PlotInit
 ***************************************************************************************************/
KymoStatus KymoApp_PlotInit(const KymoCorePlotConfig *config)
{
    return KymoCoreEndpointPlot_init(exampleEndpoint(), &sFeature, config);
}
