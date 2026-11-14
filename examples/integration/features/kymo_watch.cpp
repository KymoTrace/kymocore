/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_watch.cpp
 * @brief Watch-specific bindings and complete initialization.
 */
#include "kymo_watch.h"
#include "../detail/kymo_internal.h"
static KymoCoreEndpointWatch sFeature;
#include "../demo_values.h"
#include <string.h>
static const uint32_t WATCH_GAIN_ADDRESS = 0x20000000u;
static const uint8_t WATCH_CHANNEL = 150u;
static const KymoWatchRange RANGES[] = {{WATCH_GAIN_ADDRESS, sizeof(exampleGain)}};

/****************************************************************************************************
 * readWatch
 ***************************************************************************************************/
static uint8_t readWatch(void *user, uint32_t address, uint8_t size, uint8_t *output)
{
    uint8_t result = 0u;
    (void)user;
    if ((output != nullptr) && (address == WATCH_GAIN_ADDRESS) && (size == sizeof(exampleGain)))
    {
        memcpy(output, &exampleGain, sizeof(exampleGain));
        result = 1u;
    }
    return result;
}

/** @brief Fixed watch resources, callbacks and feature settings. */
static const KymoCoreEndpointWatchConfig sWatchConfig = {
    {RANGES, KYMO_COUNT_OF(RANGES), nullptr, 0u, readWatch, nullptr}, WATCH_CHANNEL, WATCH_CHANNEL};

/****************************************************************************************************
 * exampleWatchConfig
 ***************************************************************************************************/
const KymoCoreEndpointWatchConfig &exampleWatchConfig(void)
{
    return sWatchConfig;
}

/****************************************************************************************************
 * KymoApp_WatchInit
 ***************************************************************************************************/
KymoStatus KymoApp_WatchInit(const KymoCoreEndpointWatchConfig *config)
{
    return KymoCoreEndpointWatch_init(exampleEndpoint(), &sFeature, config);
}
