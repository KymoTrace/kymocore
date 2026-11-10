/* SPDX-License-Identifier: MIT */
/** @file kymo_bindings.cpp
 * @brief Persistent application tables and synthetic readers, independent of SDKs.
 */
#include "kymo_bindings.h"
#include "kymo_port.h"
#include <string.h>

static const uint8_t MEASUREMENT_CHANNEL = 0u;
static const uint8_t GAIN_PARAMETER_ID = 100u;
static const uint8_t MEMORY_CHANNEL = 220u;
static const uint32_t MEASUREMENT_PERIOD_MS = 50u;
static const uint32_t SAWTOOTH_PERIOD_MS = 1000u;
static const float GAIN_INITIAL = 1.0f;
static const float GAIN_MINIMUM = 0.0f;
static const float GAIN_MAXIMUM = 10.0f;
static const uint32_t WATCH_GAIN_ADDRESS = 0x20000000u;
static const uint32_t DEMO_MEMORY_TOTAL_BYTES = 4096u;
static const uint32_t DEMO_MEMORY_FREE_BYTES = 2048u;
static const size_t DEMO_MEMORY_ENTRY = 0u;
static const uint16_t MEMORY_SAMPLE_PERIOD_MS = 200u;
static const uint16_t MEMORY_INFO_PERIOD_MS = 1000u;

static float sGain = GAIN_INITIAL;
static const KymoChannel sChannels[] = { { MEASUREMENT_PERIOD_MS, MEASUREMENT_CHANNEL, 0u } };
const KymoParameter exampleParameters[EXAMPLE_PARAMETER_COUNT] = {
    { GAIN_PARAMETER_ID, &sGain, GAIN_MINIMUM, GAIN_MAXIMUM, "gain", "" }
};
static const KymoWatchRange sWatchRanges[] = { { WATCH_GAIN_ADDRESS, sizeof(sGain) } };
static const KymoMemoryEntry sMemoryEntries[] = {
    { "demo free", DEMO_MEMORY_TOTAL_BYTES, KYMO_MEMORY_HEAP_FREE, MEMORY_CHANNEL }
};

/** @brief Produce a gain-scaled sawtooth; all access runs in the main loop. */
/*******************************************************************************
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *user, uint8_t id, KymoSample *sample)
{
    uint8_t result = 0u;
    (void)user;
    if ((sample != nullptr) && (id == MEASUREMENT_CHANNEL)) {
        sample->value = sGain * (float)(examplePortClockMs(nullptr) % SAWTOOTH_PERIOD_MS) /
            (float)SAWTOOTH_PERIOD_MS;
        result = 1u;
    }
    return result;
}

/** @brief Map the documented 32-bit wire address to a native float safely. */
/*******************************************************************************
 * readWatch
 ******************************************************************************/
static uint8_t readWatch(void *user, uint32_t address, uint8_t size, uint8_t *output)
{
    uint8_t result = 0u;
    (void)user;
    if ((output != nullptr) && (address == WATCH_GAIN_ADDRESS) && (size == sizeof(sGain))) {
        memcpy(output, &sGain, sizeof(sGain));
        result = 1u;
    }
    return result;
}

/** @brief Synthetic bytes free, independent of the host allocator. */
/*******************************************************************************
 * readMemory
 ******************************************************************************/
static uint8_t readMemory(void *user, size_t index, uint32_t *bytes)
{
    uint8_t result = 0u;
    (void)user;
    if ((bytes != nullptr) && (index == DEMO_MEMORY_ENTRY)) {
        *bytes = DEMO_MEMORY_FREE_BYTES;
        result = 1u;
    }
    return result;
}

/*******************************************************************************
 * exampleTelemetryBindings
 ******************************************************************************/
KymoConfig exampleTelemetryBindings(void)
{
    KymoConfig config = {};
    config.channels = sChannels;
    config.channel_count = KYMO_COUNT_OF(sChannels);
    config.clock_ms = examplePortClockMs;
    config.sample = readSample;
    return config;
}

/*******************************************************************************
 * exampleWatchBindings
 ******************************************************************************/
KymoWatchConfig exampleWatchBindings(void)
{
    KymoWatchConfig config = {};
    config.ranges = sWatchRanges;
    config.range_count = KYMO_COUNT_OF(sWatchRanges);
    config.read = readWatch;
    /* Unknown build ID: this virtual demo address is not a real ELF symbol. */
    return config;
}

/*******************************************************************************
 * exampleMemoryBindings
 ******************************************************************************/
KymoMemoryConfig exampleMemoryBindings(void)
{
    KymoMemoryConfig config = {};
    config.entries = sMemoryEntries;
    config.count = KYMO_COUNT_OF(sMemoryEntries);
    config.period_ms = MEMORY_SAMPLE_PERIOD_MS;
    config.info_period_ms = MEMORY_INFO_PERIOD_MS;
    config.read = readMemory;
    return config;
}

/*******************************************************************************
 * exampleTraceBindings
 ******************************************************************************/
KymoTraceConfig exampleTraceBindings(void)
{
    KymoTraceConfig config = {};
    config.clock_ticks = examplePortClockTicks;
    config.session_id = EXAMPLE_TRACE_SESSION_ID;
    config.clock_domain_id = EXAMPLE_TRACE_CLOCK_DOMAIN_ID;
    /* No ISR/RTOS concurrency: every recorder access is in the main loop. */
    return config;
}
