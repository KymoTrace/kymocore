/* SPDX-License-Identifier: MIT */
/**
 * @file main.c
 * @brief Complete default-Core integration with bounded mock hardware callbacks.
 */
#include <kymo_core.h>
#include <kymo_core_plot.h>
#include <kymo_core_param.h>
#include <kymo_core_watch.h>
#include <kymo_core_memory.h>
#include <kymo_core_trace.h>
#include <assert.h>
#include "../integration/kymo_trace_records.h"
#include <stdio.h>
#include <string.h>

/** @brief Independent wire fixture sizes, identities and processing bounds. */
enum
{
    TX_BUFFER_SIZE_BYTES = 240,
    RX_BUFFER_SIZE_BYTES = 3,
    WIRE_BUFFER_SIZE_BYTES = 32768,
    WRITE_FRAGMENT_SIZE_BYTES = 7,
    MAIN_CALL_COUNT = 1000,
    SAMPLE_PERIOD_MS = 100,
    PLOT_CHANNEL = 3,
    PARAM_CHANNEL = 42,
    WATCH_CHANNEL = 5,
    MEMORY_CHANNEL = 6,
    TRACE_CONTEXT_ID = 1,
    TRACE_SYMBOL_ID = 1,
    TRACE_SESSION_ID = 1,
    TRACE_GENERATION = 1,
    TRACE_MARKER_VALUE = 123,
    MEMORY_SAMPLE_SIZE_BYTES = 512,
    MEMORY_TOTAL_SIZE_BYTES = 4096,
    ARGUMENT_COUNT = 3,
    SCENARIO_ARGUMENT_INDEX = 1,
    OUTPUT_ARGUMENT_INDEX = 2
};

/** @brief Device address used in the host's independent WATCH_SET fixture. */
#define WATCH_ADDRESS 0x20000000u

/** @brief Test sample; compared via encoded golden bytes in the Python runner. */
#define SAMPLE_VALUE 12.5f

/** @brief Parameter fixture values and comparison tolerance. */
#define INITIAL_PARAMETER_VALUE 100.0f
#define REQUESTED_PARAMETER_VALUE 200.0f
#define MAXIMUM_PARAMETER_VALUE 300.0f
#define PARAMETER_VALUE_TOLERANCE 0.01f

static uint8_t txBufferAv[TX_BUFFER_SIZE_BYTES];
static uint8_t rxBufferAv[RX_BUFFER_SIZE_BYTES];
static size_t wireSizeBytes, inputOffset, readCount, writeCount, sampleCount, memoryReadCount;
static uint32_t timeMs;

/* Golden frames: parameter ID 42 -> 200.0f, WATCH_SET slot 0, channel 5,
 * address 0x20000000, f32, period 100 ms. CRC bytes are verified independently. */
static const uint8_t inputAv[] = {
#if KYMO_EFFECTIVE_PARAMETERS
    0xa5u, 0x5au, 0x40u, 0x2au, 0x00u, 0x00u, 0x48u, 0x43u, 0xf2u,
#endif
#if KYMO_EFFECTIVE_WATCH
    0xa5u, 0x5au, 0x60u, 0x0au, 0x05u, 0x00u, 0x05u, 0x00u, 0x00u,
    0x00u, 0x20u, 0x07u, 0x64u, 0x00u, 0x9bu,
#endif
    0u};

/**
 * @brief Read fragments from the deterministic command fixture.
 */
static size_t CoreFeaturesExample_readBytes(void *pContext, uint8_t *pOutputAv,
                                            size_t capacityBytes)
{
    size_t byteCount = sizeof(inputAv) - inputOffset;
    (void)pContext;
    ++readCount;
    if (byteCount > capacityBytes)
    {
        byteCount = capacityBytes;
    }
    memcpy(pOutputAv, inputAv + inputOffset, byteCount);
    inputOffset += byteCount;
    return byteCount;
}

/**
 * @brief Capture fragmented transport writes, without sharing library internals.
 */
static size_t CoreFeaturesExample_writeBytes(void *pContext, const uint8_t *pInputAv,
                                             size_t sizeBytes)
{
    size_t byteCount = sizeBytes;
    (void)pContext;
    ++writeCount;
    if (byteCount > WRITE_FRAGMENT_SIZE_BYTES)
    {
        byteCount = WRITE_FRAGMENT_SIZE_BYTES;
    }
    (void)pInputAv; /* Replace with a bounded board-driver write. */
    wireSizeBytes += byteCount;
    return byteCount;
}

/**
 * @brief Read simulated elapsed milliseconds.
 */
static uint32_t CoreFeaturesExample_clockMs(void *pContext)
{
    (void)pContext;
    return timeMs;
}

#if KYMO_EFFECTIVE_RUNTIME

/**
 * @brief Supply a plot measurement only when Core requests it.
 */
static uint8_t CoreFeaturesExample_sample(void *pContext, uint8_t channelId, KymoSample *pSample)
{
    (void)pContext;
    assert(channelId == PLOT_CHANNEL);
    ++sampleCount;
    pSample->value = SAMPLE_VALUE;
    return 1u;
}
#endif
#if KYMO_EFFECTIVE_WATCH

/**
 * @brief Map the fixture address to a bounded host-side value read.
 */
static uint8_t CoreFeaturesExample_readValue(void *pContext, uint32_t address, uint8_t sizeBytes,
                                             uint8_t *pOutputAv)
{
    const float value = SAMPLE_VALUE;
    (void)pContext;
    assert(address == WATCH_ADDRESS && sizeBytes == sizeof(value));
    memcpy(pOutputAv, &value, sizeof(value));
    return 1u;
}
#endif
#if KYMO_EFFECTIVE_MEMORY

/**
 * @brief Supply a memory measurement only when Core requests it.
 */
static uint8_t CoreFeaturesExample_readMemory(void *pContext, size_t entryIndex,
                                              uint32_t *pSizeBytes)
{
    (void)pContext;
    assert(entryIndex == 0u);
    ++memoryReadCount;
    *pSizeBytes = MEMORY_SAMPLE_SIZE_BYTES;
    return 1u;
}
#endif
#if KYMO_EFFECTIVE_TRACE

/**
 * @brief Use the same deterministic time source for trace events.
 */
static uint64_t CoreFeaturesExample_clockTicks(void *pContext)
{
    (void)pContext;
    return timeMs;
}
#endif

/** @brief Persistent application targets and immutable feature tables. */
static const KymoChannel channelsAv[] = {{SAMPLE_PERIOD_MS, PLOT_CHANNEL, 0u}};
static float targetValue = INITIAL_PARAMETER_VALUE;
static const KymoParameter parametersAv[] = {
    KYMO_PARAM(PARAM_CHANNEL, targetValue, 0.0f, MAXIMUM_PARAMETER_VALUE, "")};
static const KymoWatchRange rangesAv[] = {{WATCH_ADDRESS, sizeof(float)}};
static const uint8_t buildIdAv[] = {1u};
static const KymoMemoryEntry entriesAv[] = {
    {"heap", MEMORY_TOTAL_SIZE_BYTES, KYMO_MEMORY_HEAP_FREE, MEMORY_CHANNEL}};

/** @brief Fixed core configuration; callbacks and tables remain alive. */
static const KymoCoreConfig core = {
    .readBytesCbk = CoreFeaturesExample_readBytes,
    .writeBytesCbk = CoreFeaturesExample_writeBytes,
    .clockMsCbk = CoreFeaturesExample_clockMs,
    .pTxBufferAv = txBufferAv,
    .txBufferSizeBytes = sizeof(txBufferAv),
    .pRxBufferAv = rxBufferAv,
    .rxBufferSizeBytes = sizeof(rxBufferAv),
};

/** @brief Fixed plot configuration; callbacks and tables remain alive. */
static const KymoCorePlotConfig plot = {
    .sampleCbk = CoreFeaturesExample_sample,
    .pChannelsAv = channelsAv,
    .channelCount = KYMO_COUNT_OF(channelsAv),
};

/** @brief Fixed parameters configuration; callbacks and tables remain alive. */
static const KymoCoreParamConfig parameters = {
    .pParametersAv = parametersAv,
    .parameterCount = KYMO_COUNT_OF(parametersAv),
};

/** @brief Fixed watch configuration; callbacks and tables remain alive. */
static const KymoCoreWatchConfig watch = {
    .readValueCbk = CoreFeaturesExample_readValue,
    .pRangesAv = rangesAv,
    .rangeCount = KYMO_COUNT_OF(rangesAv),
    .pBuildIdAv = buildIdAv,
    .buildIdSizeBytes = sizeof(buildIdAv),
    .firstChannel = WATCH_CHANNEL,
    .lastChannel = WATCH_CHANNEL,
};

/** @brief Fixed memory configuration; callbacks and tables remain alive. */
static const KymoCoreMemoryConfig memory = {
    .readBytesCbk = CoreFeaturesExample_readMemory,
    .pEntriesAv = entriesAv,
    .entryCount = KYMO_COUNT_OF(entriesAv),
    .samplePeriodMs = SAMPLE_PERIOD_MS,
};

/** @brief Fixed trace configuration; callbacks and tables remain alive. */
static const KymoCoreTraceConfig trace = {
    .clockTicksCbk = CoreFeaturesExample_clockTicks,
    .sessionId = TRACE_SESSION_ID,
    .pMetadataAv = exampleTraceMetadata,
    .metadataSizeBytes = sizeof(exampleTraceMetadata) - 1u,
    .pDictionaryAv = exampleTraceDictionary,
    .dictionarySizeBytes = sizeof(exampleTraceDictionary) - 1u,
    .generation = TRACE_GENERATION,
};

/**
 * @brief Initialize the selected features and run one common main loop.
 */
int main(void)
{
    KymoStatus status = KYMO_IDLE;
    size_t index = 0u;

    status = KymoCore_init(&core);
    if (status == KYMO_OK)
    {
        status = KymoCorePlot_init(&plot);
    }
    if (status == KYMO_OK)
    {
        status = KymoCoreParam_init(&parameters);
    }
    if (status == KYMO_OK)
    {
        status = KymoCoreWatch_init(&watch);
    }
    if (status == KYMO_OK)
    {
        status = KymoCoreMemory_init(&memory);
    }
    if (status == KYMO_OK)
    {
        status = KymoCoreTrace_init(&trace);
    }
    assert(status == KYMO_OK);
    for (index = 0u; index < MAIN_CALL_COUNT; ++index)
    {
        /* This dictionary declares a marker. Function symbols would instead use
         * KymoCoreTrace_enter/exit, or KymoCoreTraceScope in a C++ function. */
        if ((index % SAMPLE_PERIOD_MS) == 0u)
        {
            KymoCoreTrace_mark(TRACE_CONTEXT_ID, TRACE_SYMBOL_ID, (uint32_t)index);
        }
        status = KymoCore_main();
        assert(status == KYMO_OK || status == KYMO_IDLE || status == KYMO_BUSY);
        ++timeMs;
    }
    assert(targetValue > REQUESTED_PARAMETER_VALUE - PARAMETER_VALUE_TOLERANCE &&
           targetValue < REQUESTED_PARAMETER_VALUE + PARAMETER_VALUE_TOLERANCE);
    assert(sampleCount != 0u && memoryReadCount != 0u);
    assert(KymoCoreTrace_isReady() != 0u);
    assert(KymoCoreTrace_dropCount(0u) == 0u);
    printf("Five Core features active: parameter=%.0f, plot=%lu samples, memory=%lu samples, "
           "TX=%lu bytes\n",
           (double)targetValue, (unsigned long)sampleCount, (unsigned long)memoryReadCount,
           (unsigned long)wireSizeBytes);
    return 0;
}
