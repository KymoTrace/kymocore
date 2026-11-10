/* SPDX-License-Identifier: MIT */
/** @file kymo_bindings.cpp
 * @brief Persistent application tables and synthetic readers, independent of SDKs.
 */
#include "kymo_bindings.h"
#include "kymo_port.h"
#include <string.h>

static float sGain = 1.0f;
static const KymoChannel sChannels[] = { { 50u, 0u, 0u } };
const KymoParameter exampleParameters[1] = {
    { 100u, &sGain, 0.0f, 10.0f, "gain", "" }
};
static const KymoWatchRange sWatchRanges[] = { { 0x20000000u, 4u } };
static const KymoMemoryEntry sMemoryEntries[] = {
    { "demo free", 4096u, KYMO_MEMORY_HEAP_FREE, 220u }
};

/** @brief Produce a gain-scaled sawtooth; all access runs in the main loop. */
/*******************************************************************************
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *user, uint8_t id, KymoSample *sample)
{
    uint8_t result = 0u;
    (void)user;
    if ((sample != nullptr) && (id == 0u)) {
        sample->value = sGain * (float)(examplePortClockMs(nullptr) % 1000u) / 1000.0f;
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
    if ((output != nullptr) && (address == 0x20000000u) && (size == sizeof(sGain))) {
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
    if ((bytes != nullptr) && (index == 0u)) {
        *bytes = 2048u;
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
    config.period_ms = 200u;
    config.info_period_ms = 1000u;
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
    config.session_id = 1u;
    config.clock_domain_id = 0u;
    /* No ISR/RTOS concurrency: every recorder access is in the main loop. */
    return config;
}
