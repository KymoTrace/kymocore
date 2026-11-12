/* SPDX-License-Identifier: MIT */
/** @file kymo_watch.cpp
 * @brief Watch-specific bindings and complete initialization.
 */
#include "kymo_watch.h"
#include "../detail/kymo_internal.h"
static KymoEndpointWatch sFeature;
#include "../demo_values.h"
#include <string.h>
static const uint32_t WATCH_GAIN_ADDRESS = 0x20000000u;
static const uint8_t WATCH_CHANNEL = 150u;
static const KymoWatchRange RANGES[] = {{WATCH_GAIN_ADDRESS, sizeof(exampleGain)}};
/*******************************************************************************
 * readWatch
 ******************************************************************************/
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
/*******************************************************************************
 * exampleWatchConfig
 ******************************************************************************/
KymoEndpointWatchConfig exampleWatchConfig(void)
{
    KymoEndpointWatchConfig config = {};
    config.watch.ranges = RANGES;
    config.watch.range_count = KYMO_COUNT_OF(RANGES);
    config.watch.read = readWatch;
    config.first_channel = WATCH_CHANNEL;
    config.last_channel = WATCH_CHANNEL;
    return config;
}
/*******************************************************************************
 * KymoApp_WatchInit
 ******************************************************************************/
KymoStatus KymoApp_WatchInit(const KymoEndpointWatchConfig *config)
{
    return KymoEndpointWatch_Init(exampleEndpoint(), &sFeature, config);
}
