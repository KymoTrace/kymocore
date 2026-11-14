/* SPDX-License-Identifier: MIT */
/**
 * @file core_features_test.c
 * @brief C99 consumer of the complete public Core API, with no dispatcher access.
 */
#include <kymo_core.h>
#include <kymo_core_plot.h>
#include <kymo_core_param.h>
#include <kymo_core_watch.h>
#include <kymo_core_memory.h>
#include <kymo_core_trace.h>
#include <assert.h>
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
static uint8_t wireBufferAv[WIRE_BUFFER_SIZE_BYTES];
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
static size_t CoreFeaturesTest_readBytes(void *pContext, uint8_t *pOutputAv, size_t capacityBytes)
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
static size_t CoreFeaturesTest_writeBytes(void *pContext, const uint8_t *pInputAv, size_t sizeBytes)
{
    size_t byteCount = sizeBytes;
    (void)pContext;
    ++writeCount;
    if (byteCount > WRITE_FRAGMENT_SIZE_BYTES)
    {
        byteCount = WRITE_FRAGMENT_SIZE_BYTES;
    }
    assert(wireSizeBytes + byteCount <= sizeof(wireBufferAv));
    memcpy(wireBufferAv + wireSizeBytes, pInputAv, byteCount);
    wireSizeBytes += byteCount;
    return byteCount;
}

/**
 * @brief Read simulated elapsed milliseconds.
 */
static uint32_t CoreFeaturesTest_clockMs(void *pContext)
{
    (void)pContext;
    return timeMs;
}

#if KYMO_EFFECTIVE_RUNTIME

/**
 * @brief Supply a plot measurement only when Core requests it.
 */
static uint8_t CoreFeaturesTest_sample(void *pContext, uint8_t channelId, KymoSample *pSample)
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
static uint8_t CoreFeaturesTest_readValue(void *pContext, uint32_t address, uint8_t sizeBytes,
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
static uint8_t CoreFeaturesTest_readMemory(void *pContext, size_t entryIndex, uint32_t *pSizeBytes)
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
static uint64_t CoreFeaturesTest_clockTicks(void *pContext)
{
    (void)pContext;
    return timeMs;
}
#endif

/**
 * @brief Exercise registration and activation using fresh static Core state per process.
 */
int main(int argumentCount, char **pArgumentsAv)
{
    KymoCoreConfig core = {0};
    KymoStatus status = KYMO_IDLE;
    FILE *pOutput = NULL;
    size_t index = 0u;
#if KYMO_EFFECTIVE_RUNTIME
    static const KymoChannel channelsAv[] = {{SAMPLE_PERIOD_MS, PLOT_CHANNEL, 0u}};
    KymoCorePlotConfig plot = {0};
    plot.sampleCbk = CoreFeaturesTest_sample;
    plot.pChannelsAv = channelsAv;
    plot.channelCount = KYMO_COUNT_OF(channelsAv);
#endif
#if KYMO_EFFECTIVE_PARAMETERS
    static float targetValue = INITIAL_PARAMETER_VALUE;
    static const KymoParameter parametersAv[] = {
        KYMO_PARAM(PARAM_CHANNEL, targetValue, 0.0f, MAXIMUM_PARAMETER_VALUE, "")};
    KymoCoreParamConfig parameters = {parametersAv, KYMO_COUNT_OF(parametersAv)};
#endif
#if KYMO_EFFECTIVE_WATCH
    static const KymoWatchRange rangesAv[] = {{WATCH_ADDRESS, sizeof(float)}};
    static const uint8_t buildIdAv[] = {1u};
    KymoCoreWatchConfig watch = {0};
    watch.readValueCbk = CoreFeaturesTest_readValue;
    watch.pRangesAv = rangesAv;
    watch.rangeCount = KYMO_COUNT_OF(rangesAv);
    watch.pBuildIdAv = buildIdAv;
    watch.buildIdSizeBytes = sizeof(buildIdAv);
    watch.firstChannel = WATCH_CHANNEL;
    watch.lastChannel = WATCH_CHANNEL;
#endif
#if KYMO_EFFECTIVE_MEMORY
    static const KymoMemoryEntry entriesAv[] = {
        {"heap", MEMORY_TOTAL_SIZE_BYTES, KYMO_MEMORY_HEAP_FREE, MEMORY_CHANNEL}};
    KymoCoreMemoryConfig memory = {0};
    memory.readBytesCbk = CoreFeaturesTest_readMemory;
    memory.pEntriesAv = entriesAv;
    memory.entryCount = KYMO_COUNT_OF(entriesAv);
    memory.samplePeriodMs = SAMPLE_PERIOD_MS;
    KymoCoreMemory_requestInfo(); /* Safe before Init, no hidden activation. */
#endif
#if KYMO_EFFECTIVE_TRACE

    /* The transport treats metadata as opaque canonical bytes; full schema fixtures
     * and host decoding are covered by the integration and trace protocol suites. */
    static const uint8_t metadataAv[] = "{}";
    KymoCoreTraceConfig trace = {0};
    trace.clockTicksCbk = CoreFeaturesTest_clockTicks;
    trace.sessionId = TRACE_SESSION_ID;
    trace.pMetadataAv = metadataAv;
    trace.metadataSizeBytes = sizeof(metadataAv) - 1u;
    trace.pDictionaryAv = metadataAv;
    trace.dictionarySizeBytes = sizeof(metadataAv) - 1u;
    trace.generation = TRACE_GENERATION;
    KymoCoreTrace_enter(TRACE_CONTEXT_ID, TRACE_SYMBOL_ID);
    KymoCoreTrace_exit(TRACE_CONTEXT_ID, TRACE_SYMBOL_ID);
    assert(KymoCoreTrace_isReady() == 0u);
    assert(KymoCoreTrace_dropCount(0u) == 0u);
    assert(KymoCoreTrace_contextReset(TRACE_CONTEXT_ID, TRACE_GENERATION) == KYMO_BAD_CONFIG);
#endif
    assert(argumentCount == ARGUMENT_COUNT);
    core.readBytesCbk = CoreFeaturesTest_readBytes;
    core.writeBytesCbk = CoreFeaturesTest_writeBytes;
    core.clockMsCbk = CoreFeaturesTest_clockMs;
    core.pTxBufferAv = txBufferAv;
    core.txBufferSizeBytes = sizeof(txBufferAv);
    core.pRxBufferAv = rxBufferAv;
    core.rxBufferSizeBytes = sizeof(rxBufferAv);
    assert(KymoCore_init(&core) == KYMO_OK);
    if (strcmp(pArgumentsAv[SCENARIO_ARGUMENT_INDEX], "inactive") == 0)
    {
        for (index = 0u; index < MAIN_CALL_COUNT; ++index)
        {
            assert(KymoCore_main() == KYMO_IDLE);
        }
        assert(readCount == 0u && writeCount == 0u && sampleCount == 0u && memoryReadCount == 0u);
    }
    else
    {
        if (strcmp(pArgumentsAv[SCENARIO_ARGUMENT_INDEX], "late") == 0)
        {
            assert(KymoCore_main() == KYMO_IDLE);
        }
#if KYMO_EFFECTIVE_RUNTIME
        status = KymoCorePlot_init(
            strcmp(pArgumentsAv[SCENARIO_ARGUMENT_INDEX], "invalid") == 0 ? NULL : &plot);
#endif
#if KYMO_EFFECTIVE_PARAMETERS
        status = KymoCoreParam_init(
            strcmp(pArgumentsAv[SCENARIO_ARGUMENT_INDEX], "invalid") == 0 ? NULL : &parameters);
#endif
#if KYMO_EFFECTIVE_WATCH
        status = KymoCoreWatch_init(
            strcmp(pArgumentsAv[SCENARIO_ARGUMENT_INDEX], "invalid") == 0 ? NULL : &watch);
#endif
#if KYMO_EFFECTIVE_MEMORY
        status = KymoCoreMemory_init(
            strcmp(pArgumentsAv[SCENARIO_ARGUMENT_INDEX], "invalid") == 0 ? NULL : &memory);
#endif
#if KYMO_EFFECTIVE_TRACE
        status = KymoCoreTrace_init(
            strcmp(pArgumentsAv[SCENARIO_ARGUMENT_INDEX], "invalid") == 0 ? NULL : &trace);
#endif
        assert(sampleCount == 0u && memoryReadCount == 0u);
        if (strcmp(pArgumentsAv[SCENARIO_ARGUMENT_INDEX], "invalid") == 0)
        {
            assert(status == KYMO_BAD_CONFIG);
            assert(KymoCore_main() == KYMO_BAD_CONFIG);
        }
        else if (strcmp(pArgumentsAv[SCENARIO_ARGUMENT_INDEX], "late") == 0)
        {
            assert(status == KYMO_REJECTED);
            assert(KymoCore_main() == KYMO_IDLE);
        }
        else
        {
            assert(status == KYMO_OK);
#if KYMO_EFFECTIVE_TRACE
            KymoCoreTrace_enter(TRACE_CONTEXT_ID, TRACE_SYMBOL_ID);
            KymoCoreTrace_mark(TRACE_CONTEXT_ID, TRACE_SYMBOL_ID, TRACE_MARKER_VALUE);
            KymoCoreTrace_exit(TRACE_CONTEXT_ID, TRACE_SYMBOL_ID);
#if KYMO_EFFECTIVE_TRACE_ISR
            KymoCoreTrace_isrEnter(TRACE_CONTEXT_ID + 1u);
            KymoCoreTrace_isrExit(TRACE_CONTEXT_ID + 1u);
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER
            KymoCoreTrace_taskCreate(TRACE_CONTEXT_ID, TRACE_GENERATION);
            KymoCoreTrace_scheduler(TRACE_CONTEXT_ID, TRACE_CONTEXT_ID);
            KymoCoreTrace_coreState(TRACE_CONTEXT_ID, 0u, 0u);
            KymoCoreTrace_taskEnd(TRACE_CONTEXT_ID, TRACE_GENERATION);
#endif
#if KYMO_EFFECTIVE_TRACE_FLOW
            KymoCoreTrace_flowSend(TRACE_CONTEXT_ID, TRACE_SYMBOL_ID, TRACE_MARKER_VALUE);
            KymoCoreTrace_flowReceive(TRACE_CONTEXT_ID, TRACE_SYMBOL_ID, TRACE_MARKER_VALUE);
#endif
#endif
            for (index = 0u; index < MAIN_CALL_COUNT; ++index)
            {
                status = KymoCore_main();
                assert(status == KYMO_OK || status == KYMO_BUSY || status == KYMO_IDLE);
                ++timeMs;
            }
#if KYMO_EFFECTIVE_RUNTIME
            assert(sampleCount != 0u);
#endif
#if KYMO_EFFECTIVE_PARAMETERS
            assert(targetValue > REQUESTED_PARAMETER_VALUE - PARAMETER_VALUE_TOLERANCE &&
                   targetValue < REQUESTED_PARAMETER_VALUE + PARAMETER_VALUE_TOLERANCE);
#endif
#if KYMO_EFFECTIVE_MEMORY
            assert(memoryReadCount != 0u);
            KymoCoreMemory_requestInfo();
#endif
#if KYMO_EFFECTIVE_TRACE
            assert(KymoCoreTrace_isReady() != 0u);
            assert(KymoCoreTrace_dropCount(0u) == 0u);
            assert(KymoCoreTrace_contextReset(TRACE_CONTEXT_ID, TRACE_GENERATION) == KYMO_OK);
#endif
            assert(wireSizeBytes != 0u);
        }
    }
    pOutput = fopen(pArgumentsAv[OUTPUT_ARGUMENT_INDEX], "wb");
    assert(pOutput != NULL);
    assert(fwrite(wireBufferAv, 1u, wireSizeBytes, pOutput) == wireSizeBytes);
    assert(fclose(pOutput) == 0);
    return 0;
}
