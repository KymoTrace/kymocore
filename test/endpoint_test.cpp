/* SPDX-License-Identifier: MIT */
/** @file endpoint_test.cpp
 * @brief Public Endpoint tests, built with each feature independently and together.
 */
#include <endpoint/kymo_endpoint.h>
#include <endpoint/features/kymo_plot.h>
#include <endpoint/features/kymo_param.h>
#include <endpoint/features/kymo_watch.h>
#include <endpoint/features/kymo_memory.h>
#include <endpoint/features/kymo_trace.h>
#include <common/embedded_bytes.h>
#include <endpoint/detail/kymo_internal.h>
#include <common/embedded_crc.h>
#include <assert.h>
#include <string.h>
#include <math.h>

static const size_t FRAME_CAPACITY = 240u;
static const size_t WIRE_CAPACITY = 32768u;
static const uint32_t TEST_CYCLES = 200u;
static const uint32_t DRAIN_LIMIT = 4000u;
static const uint8_t PLOT_CHANNEL = 3u;
static const uint8_t PARAM_CHANNEL = 4u;
static const uint8_t WATCH_CHANNEL = 5u;
static const uint8_t MEMORY_CHANNEL = 6u;

struct Fixture {
    KymoEndpoint endpoint;
    uint8_t storage[FRAME_CAPACITY];
    uint8_t rx[16];
    uint8_t input[64];
    size_t input_length;
    size_t input_offset;
    uint8_t wire[WIRE_CAPACITY];
    size_t wire_length;
    size_t writes;
    size_t reads;
    size_t services;
    size_t sample_calls;
    uint32_t ticks;
    uint8_t busy;
    uint8_t reject;
    uint8_t violate;
    uint8_t bad_read;
    uint8_t bad_sample;
    uint8_t borrow;
    const uint8_t *borrowed;
    size_t borrowed_length;
    uint8_t snapshot[FRAME_CAPACITY];
#if KYMO_EFFECTIVE_RUNTIME
    KymoEndpointPlot plot;
#endif
#if KYMO_EFFECTIVE_PARAMETERS
    KymoEndpointParam param;
    float gain;
    KymoParameter parameter;
#endif
#if KYMO_EFFECTIVE_WATCH
    KymoEndpointWatch watch;
#endif
#if KYMO_EFFECTIVE_MEMORY
    KymoEndpointMemory memory;
#endif
#if KYMO_EFFECTIVE_TRACE
    KymoEndpointTrace trace;
#endif
};

/*******************************************************************************
 * clockMs
 ******************************************************************************/
static uint32_t clockMs(void *user)
{
    return static_cast<Fixture *>(user)->ticks;
}
/*******************************************************************************
 * writeBytes
 ******************************************************************************/
static size_t writeBytes(void *user, const uint8_t *bytes, size_t length)
{
    Fixture *fixture = static_cast<Fixture *>(user);
    size_t accepted = 0u;
    assert(fixture->busy == 0u);
    ++fixture->writes;
    if (fixture->violate != 0u) {
        accepted = length + 1u;
    } else if (fixture->reject == 0u) {
        accepted = fixture->borrow != 0u ? 1u : length;
        if (fixture->borrow != 0u) {
            fixture->borrowed = bytes;
            fixture->borrowed_length = accepted;
            memcpy(fixture->snapshot, bytes, accepted);
            fixture->busy = 1u;
        } else {
            assert(fixture->wire_length + accepted <= sizeof(fixture->wire));
            memcpy(fixture->wire + fixture->wire_length, bytes, accepted);
            fixture->wire_length += accepted;
        }
    }
    return accepted;
}
/*******************************************************************************
 * busyPort
 ******************************************************************************/
static uint8_t busyPort(void *user)
{
    return static_cast<Fixture *>(user)->busy;
}
/*******************************************************************************
 * readBytes
 ******************************************************************************/
static size_t readBytes(void *user, uint8_t *bytes, size_t capacity)
{
    Fixture *fixture = static_cast<Fixture *>(user);
    size_t length = fixture->input_length - fixture->input_offset;
    length = length < capacity ? length : capacity;
    memcpy(bytes, fixture->input + fixture->input_offset, length);
    fixture->input_offset += length;
    ++fixture->reads;
    return fixture->bad_read != 0u ? capacity + 1u : length;
}
/*******************************************************************************
 * servicePort
 ******************************************************************************/
static void servicePort(void *user)
{
    Fixture *fixture = static_cast<Fixture *>(user);
    ++fixture->ticks;
    ++fixture->services;
}
/*******************************************************************************
 * baseConfig
 ******************************************************************************/
static KymoEndpointConfig baseConfig(Fixture *fixture)
{
    KymoEndpointConfig config = {};
    config.tx.buffer = fixture->storage;
    config.tx.capacity = sizeof(fixture->storage);
    config.tx.write = writeBytes;
    config.tx.busy = busyPort;
    config.tx.transport_user = fixture;
    config.read = readBytes;
    config.service = servicePort;
    config.rx_buffer = fixture->rx;
    config.rx_capacity = sizeof(fixture->rx);
    config.clock_ms = clockMs;
    config.clock_user = fixture;
    return config;
}
#if KYMO_EFFECTIVE_RUNTIME
static const KymoChannel CHANNELS[] = { { 0u, PLOT_CHANNEL,
#if KYMO_EFFECTIVE_X
    KYMO_FLAG_X
#else
    0u
#endif
} };
/*******************************************************************************
 * samplePlot
 ******************************************************************************/
static uint8_t samplePlot(void *user, uint8_t id, KymoSample *sample)
{
    Fixture *fixture = static_cast<Fixture *>(user);
    assert(id == PLOT_CHANNEL);
    ++fixture->sample_calls;
    sample->x = 2.0f;
    sample->value = fixture->bad_sample != 0u ? NAN : 3.0f;
    return 1u;
}
#endif
#if KYMO_EFFECTIVE_MEMORY
static const KymoMemoryEntry ENTRIES[] = { { "heap", 1024u, KYMO_MEMORY_HEAP_FREE, MEMORY_CHANNEL } };
/*******************************************************************************
 * sampleMemory
 ******************************************************************************/
static uint8_t sampleMemory(void *user, size_t index, uint32_t *bytes)
{
    (void)user;
    (void)index;
    *bytes = 512u;
    return 1u;
}
#endif
#if KYMO_EFFECTIVE_TRACE
static const uint8_t TEST_METADATA[] = "{}";
/*******************************************************************************
 * clockTicks
 ******************************************************************************/
static uint64_t clockTicks(void *user)
{
    return static_cast<Fixture *>(user)->ticks;
}
#endif
#if KYMO_EFFECTIVE_PARAMETERS || KYMO_EFFECTIVE_WATCH
/*******************************************************************************
 * appendControl
 ******************************************************************************/
static void appendControl(Fixture *fixture, const uint8_t *payload, uint8_t length)
{
    uint8_t *frame = fixture->input + fixture->input_length;
    assert(fixture->input_length + length + 5u <= sizeof(fixture->input));
    frame[0] = KYMO_SYNC_0;
    frame[1] = KYMO_SYNC_1;
    frame[2] = KYMO_DESCRIPTOR_PARAM;
    frame[3] = length;
    memcpy(frame + 4u, payload, length);
    frame[4u + length] = emb_crc8_msb(frame + 2u, 2u + length, 0x07u, 0u);
    fixture->input_length += 5u + length;
}
#endif
/*******************************************************************************
 * initFeatures
 ******************************************************************************/
static void initFeatures(Fixture *fixture)
{
#if KYMO_EFFECTIVE_TRACE
    KymoEndpointTraceConfig trace = {};
    trace.recorder.clock_ticks = clockTicks;
    trace.recorder.user = fixture;
    trace.recorder.session_id = 1u;
    trace.metadata = TEST_METADATA;
    trace.metadata_length = sizeof(TEST_METADATA) - 1u;
    trace.dictionary = TEST_METADATA;
    trace.dictionary_length = sizeof(TEST_METADATA) - 1u;
    trace.generation = 1u;
    trace.health_period_ticks = 1u;
    assert(KymoEndpointTrace_Init(&fixture->endpoint, &fixture->trace, &trace) == KYMO_OK);
    /* Events before first Main must follow both administrative records on wire. */
    KYMO_ENDPOINT_TRACE_MARK(&fixture->trace, 1u, 1u, 1u);
#endif
#if KYMO_EFFECTIVE_MEMORY
    KymoEndpointMemoryConfig memory = {};
    memory.entries = ENTRIES;
    memory.count = KYMO_COUNT_OF(ENTRIES);
    memory.period_ms = 1u;
    memory.info_period_ms = 1u;
    memory.read = sampleMemory;
    assert(KymoEndpointMemory_Init(&fixture->endpoint, &fixture->memory, &memory) == KYMO_OK);
#endif
#if KYMO_EFFECTIVE_WATCH
    static const KymoWatchRange ranges[] = { { 0x20000000u, sizeof(float) } };
    KymoEndpointWatchConfig watch = {};
    watch.watch.ranges = ranges;
    watch.watch.range_count = KYMO_COUNT_OF(ranges);
    watch.first_channel = WATCH_CHANNEL;
    watch.last_channel = WATCH_CHANNEL;
    assert(KymoEndpointWatch_Init(&fixture->endpoint, &fixture->watch, &watch) == KYMO_OK);
    const uint8_t info = KYMO_WATCH_OP_INFO;
    appendControl(fixture, &info, sizeof(info));
#endif
#if KYMO_EFFECTIVE_PARAMETERS
    KymoEndpointParamConfig param = {};
    fixture->parameter.id = PARAM_CHANNEL;
    fixture->parameter.target = &fixture->gain;
    fixture->parameter.maximum = 10.0f;
    param.table = &fixture->parameter;
    param.count = 1u;
    assert(KymoEndpointParam_Init(&fixture->endpoint, &fixture->param, &param) == KYMO_OK);
    const uint8_t list = KYMO_PARAM_OP_LIST;
    appendControl(fixture, &list, sizeof(list));
#endif
#if KYMO_EFFECTIVE_RUNTIME
    KymoEndpointPlotConfig plot = {};
    plot.channels = CHANNELS;
    plot.channel_count = KYMO_COUNT_OF(CHANNELS);
    plot.sample = samplePlot;
    plot.sample_user = fixture;
    assert(KymoEndpointPlot_Init(&fixture->endpoint, &fixture->plot, &plot) == KYMO_OK);
#endif
}
/*******************************************************************************
 * runCycle
 ******************************************************************************/
static KymoStatus runCycle(Fixture *fixture)
{
    size_t writes = fixture->writes;
    size_t services = fixture->services;
    KymoStatus status = KymoEndpoint_Main(&fixture->endpoint);
    assert(fixture->writes <= writes + 1u);
    assert(fixture->services <= services + 1u);
    return status;
}
/*******************************************************************************
 * releaseBorrow
 ******************************************************************************/
static void releaseBorrow(Fixture *fixture)
{
    if (fixture->borrowed != nullptr) {
        assert(memcmp(fixture->borrowed, fixture->snapshot, fixture->borrowed_length) == 0);
        assert(fixture->wire_length + fixture->borrowed_length <= sizeof(fixture->wire));
        memcpy(fixture->wire + fixture->wire_length, fixture->borrowed, fixture->borrowed_length);
        fixture->wire_length += fixture->borrowed_length;
        fixture->borrowed = nullptr;
        fixture->borrowed_length = 0u;
        fixture->busy = 0u;
    }
}
/*******************************************************************************
 * testLifecycle
 ******************************************************************************/
static void testLifecycle(uint8_t borrow)
{
    Fixture fixture = {};
    KymoEndpointConfig config = baseConfig(&fixture);
    fixture.borrow = borrow;
    assert(KymoEndpoint_Main(&fixture.endpoint) == KYMO_BAD_CONFIG);
    assert(KymoEndpoint_Init(&fixture.endpoint, &config) == KYMO_OK);
    assert(KymoEndpoint_RequestStop(&fixture.endpoint) == KYMO_REJECTED);
    initFeatures(&fixture);
    assert(fixture.writes == 0u && fixture.services == 0u && fixture.reads == 0u);
    for (uint32_t cycle = 0u; cycle < TEST_CYCLES; ++cycle) {
        KymoStatus status = runCycle(&fixture);
        assert(status == KYMO_OK || status == KYMO_BUSY || status == KYMO_IDLE);
        if (fixture.borrowed != nullptr) {
            size_t writes = fixture.writes;
            assert(runCycle(&fixture) == KYMO_BUSY && fixture.writes == writes);
            releaseBorrow(&fixture);
        }
    }
    assert(KymoEndpoint_Init(&fixture.endpoint, &config) == KYMO_REJECTED);
    assert(KymoEndpoint_RequestStop(&fixture.endpoint) == KYMO_OK);
    size_t reads = fixture.reads;
    uint32_t cycles = 0u;
    while ((fixture.endpoint.phase != KYMO_ENDPOINT_STOPPED) && (cycles < DRAIN_LIMIT)) {
        KymoStatus status = runCycle(&fixture);
        assert(status == KYMO_BUSY || status == KYMO_IDLE);
        releaseBorrow(&fixture);
        ++cycles;
    }
    assert(fixture.endpoint.phase == KYMO_ENDPOINT_STOPPED);
    assert(fixture.reads == reads && fixture.borrowed == nullptr);
    size_t services = fixture.services;
    assert(KymoEndpoint_Main(&fixture.endpoint) == KYMO_IDLE);
    assert(fixture.services == services);
#if KYMO_EFFECTIVE_TRACE
    assert(fixture.trace.end_queued != 0u);
    assert(KymoEndpointTrace_Recorder(&fixture.trace) == nullptr);
#if !KYMO_EFFECTIVE_RUNTIME && !KYMO_EFFECTIVE_PARAMETERS && !KYMO_EFFECTIVE_WATCH && !KYMO_EFFECTIVE_MEMORY
    size_t offset = 0u;
    size_t frames = 0u;
    uint8_t foundEvent = 0u;
    uint8_t lastKind = 0u;
    while (offset < fixture.wire_length) {
        assert(fixture.wire_length - offset >= KYMO_TRACE_ENVELOPE_SIZE);
        const uint8_t *frame = fixture.wire + offset;
        size_t length = KYMO_TRACE_ENVELOPE_SIZE +
            (emb_read_u32_le(frame + 4u) & UINT32_C(0xffff));
        assert(length <= fixture.wire_length - offset);
        lastKind = frame[3];
        if (frames == 0u) {
            assert(lastKind == KYMO_TRACE_PAYLOAD_SESSION_META);
        } else if (frames == 1u) {
            assert(lastKind == KYMO_TRACE_PAYLOAD_DICTIONARY);
        }
        if (lastKind == KYMO_TRACE_PAYLOAD_BATCH) {
            assert(frames >= 2u);
            foundEvent = 1u;
        }
        ++frames;
        offset += length;
    }
    assert(foundEvent != 0u && lastKind == KYMO_TRACE_PAYLOAD_SESSION_END);
#endif
#endif
}
/*******************************************************************************
 * testInvalidAndIndependent
 ******************************************************************************/
static void testInvalidAndIndependent(void)
{
    Fixture first = {};
    Fixture second = {};
    KymoEndpointConfig one = baseConfig(&first);
    KymoEndpointConfig two = baseConfig(&second);
    assert(KymoEndpoint_Init(nullptr, &one) == KYMO_BAD_CONFIG);
    assert(KymoEndpoint_Init(&first.endpoint, nullptr) == KYMO_BAD_CONFIG);
    assert(KymoEndpoint_Init(&first.endpoint, &one) == KYMO_OK);
    assert(KymoEndpoint_Main(&first.endpoint) == KYMO_BAD_CONFIG); /* No features. */
    assert(first.services == 0u && first.writes == 0u);
    assert(KymoEndpoint_Init(&second.endpoint, &two) == KYMO_OK);
    initFeatures(&second);
    for (uint32_t cycle = 0u; cycle < TEST_CYCLES; ++cycle) {
        (void)runCycle(&second);
    }
    assert(second.services == TEST_CYCLES && first.services == 0u);
}
/*******************************************************************************
 * invalidFeature
 ******************************************************************************/
static KymoStatus invalidFeature(Fixture *fixture)
{
    KymoStatus status = KYMO_BAD_CONFIG;
#if KYMO_EFFECTIVE_RUNTIME
    status = KymoEndpointPlot_Init(&fixture->endpoint, &fixture->plot, nullptr);
#elif KYMO_EFFECTIVE_PARAMETERS
    status = KymoEndpointParam_Init(&fixture->endpoint, &fixture->param, nullptr);
#elif KYMO_EFFECTIVE_WATCH
    status = KymoEndpointWatch_Init(&fixture->endpoint, &fixture->watch, nullptr);
#elif KYMO_EFFECTIVE_MEMORY
    status = KymoEndpointMemory_Init(&fixture->endpoint, &fixture->memory, nullptr);
#elif KYMO_EFFECTIVE_TRACE
    status = KymoEndpointTrace_Init(&fixture->endpoint, &fixture->trace, nullptr);
#endif
    return status;
}
/*******************************************************************************
 * testSetupErrors
 ******************************************************************************/
static void testSetupErrors(void)
{
    Fixture fixture = {};
    KymoEndpointConfig config = baseConfig(&fixture);
    assert(invalidFeature(&fixture) == KYMO_BAD_CONFIG);
    assert(fixture.endpoint.phase == KYMO_ENDPOINT_UNINITIALIZED);
    config.rx_buffer = config.tx.buffer;
    assert(KymoEndpoint_Init(&fixture.endpoint, &config) == KYMO_BAD_CONFIG);
    config.rx_buffer = fixture.rx;
    assert(KymoEndpoint_Init(&fixture.endpoint, &config) == KYMO_OK);
    assert(invalidFeature(&fixture) == KYMO_BAD_CONFIG);
    assert(KymoEndpoint_Main(&fixture.endpoint) == KYMO_BAD_CONFIG);
    assert(fixture.endpoint.phase == KYMO_ENDPOINT_FAULT && fixture.services == 0u);
}
/*******************************************************************************
 * testBackpressureAndFault
 ******************************************************************************/
static void testBackpressureAndFault(void)
{
    Fixture fixture = {};
    KymoEndpointConfig config = baseConfig(&fixture);
    assert(KymoEndpoint_Init(&fixture.endpoint, &config) == KYMO_OK);
    initFeatures(&fixture);
    fixture.busy = 1u;
    for (uint32_t cycle = 0u; cycle < TEST_CYCLES; ++cycle) {
        (void)runCycle(&fixture);
    }
    assert(fixture.writes == 0u && fixture.endpoint.tx.length != 0u);
    assert(fixture.input_offset == fixture.input_length); /* RX progresses under busy. */
    assert(invalidFeature(&fixture) == KYMO_REJECTED);
    assert(fixture.endpoint.phase == KYMO_ENDPOINT_RUNNING);
    fixture.busy = 0u;
    fixture.reject = 1u;
    uint8_t queued[FRAME_CAPACITY] = {};
    size_t length = fixture.endpoint.tx.length;
    memcpy(queued, fixture.storage, length);
    assert(runCycle(&fixture) == KYMO_BUSY);
    assert(memcmp(queued, fixture.storage, length) == 0);
    fixture.reject = 0u;
    fixture.violate = 1u;
    assert(runCycle(&fixture) == KYMO_IO_ERROR);
    assert(fixture.endpoint.error_feature == KYMO_ENDPOINT_TRANSPORT);
    size_t calls = fixture.services;
    size_t writes = fixture.writes;
    assert(KymoEndpoint_Main(&fixture.endpoint) == KYMO_IO_ERROR);
    assert(KymoEndpoint_RequestStop(&fixture.endpoint) == KYMO_IO_ERROR);
    assert(fixture.services == calls && fixture.writes == writes);
}
#if KYMO_EFFECTIVE_PARAMETERS || KYMO_EFFECTIVE_WATCH
/*******************************************************************************
 * testReadViolation
 ******************************************************************************/
static void testReadViolation(void)
{
    Fixture fixture = {};
    KymoEndpointConfig config = baseConfig(&fixture);
    assert(KymoEndpoint_Init(&fixture.endpoint, &config) == KYMO_OK);
    initFeatures(&fixture);
    fixture.bad_read = 1u;
    assert(KymoEndpoint_Main(&fixture.endpoint) == KYMO_IO_ERROR);
    assert(fixture.writes == 0u);
}
#endif
#if KYMO_EFFECTIVE_WATCH
/*******************************************************************************
 * testWatchChannelPolicy
 ******************************************************************************/
static void testWatchChannelPolicy(void)
{
    Fixture fixture = {};
    KymoEndpointConfig config = baseConfig(&fixture);
    assert(KymoEndpoint_Init(&fixture.endpoint, &config) == KYMO_OK);
    initFeatures(&fixture);
    fixture.input_length = 0u;
    uint8_t request[] = { KYMO_WATCH_OP_SET, 0u, PLOT_CHANNEL,
        0u, 0u, 0u, 0x20u, KYMO_WATCH_TYPE_F32, 100u, 0u };
    appendControl(&fixture, request, sizeof(request));
    (void)KymoEndpoint_Main(&fixture.endpoint);
    assert(fixture.watch.watch.rejected == 1u);
    assert(fixture.watch.watch.slots[0].type == 0u);
    assert(fixture.watch.watch.ack_status[0] == KYMO_WATCH_STATUS_OUT_OF_RANGE);
    request[2] = WATCH_CHANNEL;
    fixture.input_length = 0u;
    fixture.input_offset = 0u;
    appendControl(&fixture, request, sizeof(request));
    /* Do not sample the fake physical address: keep the physical TX occupied. */
    fixture.busy = 1u;
    (void)KymoEndpoint_Main(&fixture.endpoint);
    assert(fixture.watch.watch.accepted == 1u);
    assert(fixture.watch.watch.slots[0].channel == WATCH_CHANNEL);
}
#endif
#if KYMO_EFFECTIVE_RUNTIME
/*******************************************************************************
 * plotConfig
 ******************************************************************************/
static KymoEndpointPlotConfig plotConfig(Fixture *fixture)
{
    KymoEndpointPlotConfig config = {};
    config.channels = CHANNELS;
    config.channel_count = KYMO_COUNT_OF(CHANNELS);
    config.sample = samplePlot;
    config.sample_user = fixture;
    return config;
}
/*******************************************************************************
 * testPlotConfigurationAndRecovery
 ******************************************************************************/
static void testPlotConfigurationAndRecovery(void)
{
    Fixture small = {};
    KymoEndpointConfig base = baseConfig(&small);
    KymoEndpointPlotConfig plot = plotConfig(&small);
    base.tx.capacity = KYMO_FRAME_MAX_SIZE - 1u;
    assert(KymoEndpoint_Init(&small.endpoint, &base) == KYMO_OK);
    assert(KymoEndpointPlot_Init(&small.endpoint, &small.plot, &plot) == KYMO_BAD_CONFIG);
    assert(small.endpoint.phase == KYMO_ENDPOINT_FAULT && small.writes == 0u);
    Fixture fixture = {};
    base = baseConfig(&fixture);
    plot = plotConfig(&fixture);
    assert(KymoEndpoint_Init(&fixture.endpoint, &base) == KYMO_OK);
    assert(KymoEndpointPlot_Init(&fixture.endpoint, &fixture.plot, &plot) == KYMO_OK);
    fixture.bad_sample = 1u;
    assert(KymoEndpoint_Main(&fixture.endpoint) == KYMO_BAD_SAMPLE);
    assert(fixture.endpoint.phase == KYMO_ENDPOINT_RUNNING);
    fixture.bad_sample = 0u;
    assert(KymoEndpoint_Main(&fixture.endpoint) == KYMO_BUSY);
    assert(KymoEndpoint_Main(&fixture.endpoint) == KYMO_BUSY);
    assert(fixture.writes == 1u);
    Fixture duplicate = {};
    base = baseConfig(&duplicate);
    plot = plotConfig(&duplicate);
    KymoEndpointPlot other = {};
    assert(KymoEndpoint_Init(&duplicate.endpoint, &base) == KYMO_OK);
    assert(KymoEndpointPlot_Init(&duplicate.endpoint, &duplicate.plot, &plot) == KYMO_OK);
    assert(KymoEndpointPlot_Init(&duplicate.endpoint, &other, &plot) == KYMO_REJECTED);
    assert(duplicate.endpoint.phase == KYMO_ENDPOINT_FAULT && other.binding.endpoint == nullptr);
}
#if KYMO_EFFECTIVE_WATCH
/*******************************************************************************
 * testChannelCollision
 ******************************************************************************/
static void testChannelCollision(uint8_t watchFirst)
{
    Fixture fixture = {};
    KymoEndpointConfig base = baseConfig(&fixture);
    KymoEndpointPlotConfig plot = plotConfig(&fixture);
    static const KymoWatchRange ranges[] = { { 0x20000000u, sizeof(float) } };
    KymoEndpointWatchConfig watch = {};
    watch.watch.ranges = ranges;
    watch.watch.range_count = KYMO_COUNT_OF(ranges);
    watch.first_channel = PLOT_CHANNEL;
    watch.last_channel = PLOT_CHANNEL;
    assert(KymoEndpoint_Init(&fixture.endpoint, &base) == KYMO_OK);
    if (watchFirst != 0u) {
        assert(KymoEndpointWatch_Init(&fixture.endpoint, &fixture.watch, &watch) == KYMO_OK);
        assert(KymoEndpointPlot_Init(&fixture.endpoint, &fixture.plot, &plot) == KYMO_BAD_CONFIG);
    } else {
        assert(KymoEndpointPlot_Init(&fixture.endpoint, &fixture.plot, &plot) == KYMO_OK);
        assert(KymoEndpointWatch_Init(&fixture.endpoint, &fixture.watch, &watch) == KYMO_BAD_CONFIG);
    }
    assert(KymoEndpoint_Main(&fixture.endpoint) == KYMO_BAD_CONFIG);
    assert(fixture.services == 0u && fixture.writes == 0u);
}
#endif
#endif
#if KYMO_EFFECTIVE_TRACE
/*******************************************************************************
 * testMetadataRepeat
 ******************************************************************************/
static void testMetadataRepeat(uint32_t period, uint8_t borrow)
{
    Fixture fixture = {};
    KymoEndpointConfig base = baseConfig(&fixture);
    KymoEndpointTraceConfig config = {};
    static const uint8_t largeRecord[500] = { '{', '}' };
    fixture.borrow = borrow;
    config.recorder.clock_ticks = clockTicks;
    config.recorder.user = &fixture;
    config.recorder.session_id = 1u;
    config.metadata = largeRecord;
    config.metadata_length = sizeof(largeRecord);
    config.dictionary = largeRecord;
    config.dictionary_length = sizeof(largeRecord);
    config.generation = 1u;
    config.metadata_period_ticks = period;
    assert(KymoEndpoint_Init(&fixture.endpoint, &base) == KYMO_OK);
    assert(KymoEndpointTrace_Init(&fixture.endpoint, &fixture.trace, &config) == KYMO_OK);
    uint8_t wasReady = 0u;
    uint32_t pairs = 0u;
    uint8_t previousStage = 0u;
    const uint32_t limit = borrow != 0u ? 6000u : 300u;
    for (uint32_t cycle = 0u; cycle < limit; ++cycle) {
        if ((cycle % 100u) == 0u) {
            KYMO_ENDPOINT_TRACE_MARK(&fixture.trace, 1u, 1u, cycle);
        }
        (void)runCycle(&fixture);
        releaseBorrow(&fixture);
        uint8_t ready = KymoEndpointTrace_IsReady(&fixture.trace);
        assert(wasReady == 0u || ready != 0u);
        wasReady = ready;
        if (fixture.trace.metadata_stage == 2u && previousStage != 2u) {
            ++pairs;
        }
        previousStage = fixture.trace.metadata_stage;
    }
    assert(wasReady != 0u);
    assert(period == 0u ? pairs == 1u : pairs >= 2u);
    /* Stop during the next refresh: finite drain, no new repetition. */
    if (period != 0u) {
        uint32_t wait = 0u;
        while ((fixture.trace.metadata_stage == 2u) && (wait < DRAIN_LIMIT)) {
            (void)runCycle(&fixture);
            releaseBorrow(&fixture);
            ++wait;
        }
        assert(wait < DRAIN_LIMIT);
    }
    assert(KymoEndpoint_RequestStop(&fixture.endpoint) == KYMO_OK);
    for (uint32_t cycle = 0u; cycle < DRAIN_LIMIT && fixture.endpoint.phase != KYMO_ENDPOINT_STOPPED; ++cycle) {
        (void)runCycle(&fixture);
        releaseBorrow(&fixture);
    }
    assert(fixture.endpoint.phase == KYMO_ENDPOINT_STOPPED);
    assert(KymoEndpointTrace_IsReady(&fixture.trace) == 0u);
    size_t offset = 0u;
    size_t events = 0u;
    uint8_t lastKind = 0u;
    while (offset < fixture.wire_length) {
        const uint8_t *frame = fixture.wire + offset;
        assert(fixture.wire_length - offset >= KYMO_TRACE_ENVELOPE_SIZE);
        size_t length = KYMO_TRACE_ENVELOPE_SIZE + (emb_read_u32_le(frame + 4u) & UINT32_C(0xffff));
        assert(length <= fixture.wire_length - offset);
        assert(KymoTrace_Crc16(frame + 2u, length - 4u) ==
            (uint16_t)(frame[length - 2u] | ((uint16_t)frame[length - 1u] << 8u)));
        lastKind = frame[3];
        if (lastKind == KYMO_TRACE_PAYLOAD_BATCH) { ++events; }
        offset += length;
    }
    assert(events > 0u && lastKind == KYMO_TRACE_PAYLOAD_SESSION_END);
}
#endif
/*******************************************************************************
 * testControlSampleTime
 ******************************************************************************/
static void testControlSampleTime(void)
{
    Fixture fixture = {};
    KymoEndpointConfig config = baseConfig(&fixture);
    KymoDataPoint point = {};
    const uint32_t SAMPLE_TIME_MS = 123456u;
    point.id = WATCH_CHANNEL;
    point.value = 1.25f;
    assert(KymoEndpoint_Init(&fixture.endpoint, &config) == KYMO_OK);
    assert(kymoEndpointSample(&fixture.endpoint, KYMO_IDLE, &point, SAMPLE_TIME_MS) == KYMO_IDLE);
    assert(fixture.endpoint.tx.length == 0u);
    assert(kymoEndpointSample(&fixture.endpoint, KYMO_OK, &point, SAMPLE_TIME_MS) == KYMO_OK);
#if KYMO_EFFECTIVE_TIMESTAMP
    assert((fixture.storage[2] & KYMO_FLAG_TIMESTAMP) != 0u);
    const size_t TIMESTAMP_OFFSET = 8u;
    assert(emb_read_u32_le(fixture.storage + TIMESTAMP_OFFSET) == SAMPLE_TIME_MS);
#else
    assert((fixture.storage[2] & KYMO_FLAG_TIMESTAMP) == 0u);
#endif
    assert(point.flags == 0u); /* The caller's point remains unchanged. */
}
#if KYMO_EFFECTIVE_TIMESTAMP && KYMO_EFFECTIVE_RUNTIME && KYMO_EFFECTIVE_PARAMETERS
/*******************************************************************************
 * testSharedDeviceClock
 ******************************************************************************/
static void testSharedDeviceClock(uint32_t initializedAt, uint32_t sampledAt)
{
    Fixture fixture = {};
    KymoEndpointConfig config = baseConfig(&fixture);
    const KymoChannel channel = { 0u, PLOT_CHANNEL, KYMO_FLAG_TIMESTAMP };
    KymoEndpointPlotConfig plot = {};
    KymoEndpointParamConfig param = {};
    const uint8_t list = KYMO_PARAM_OP_LIST;
    const size_t TIMESTAMP_OFFSET = 8u;
    size_t offset = 0u;
    uint8_t seenPlot = 0u;
    uint8_t seenParam = 0u;
    fixture.ticks = initializedAt;
    config.service = nullptr; /* Hold the clock fixed while dispatching both features. */
    plot.channels = &channel;
    plot.channel_count = 1u;
    plot.sample = samplePlot;
    plot.sample_user = &fixture;
    fixture.parameter.id = PARAM_CHANNEL;
    fixture.parameter.target = &fixture.gain;
    fixture.parameter.maximum = 10.0f;
    param.table = &fixture.parameter;
    param.count = 1u;
    assert(KymoEndpoint_Init(&fixture.endpoint, &config) == KYMO_OK);
    assert(KymoEndpointPlot_Init(&fixture.endpoint, &fixture.plot, &plot) == KYMO_OK);
    assert(KymoEndpointParam_Init(&fixture.endpoint, &fixture.param, &param) == KYMO_OK);
    appendControl(&fixture, &list, sizeof(list));
    fixture.ticks = sampledAt;
    for (uint32_t cycle = 0u; cycle < TEST_CYCLES; ++cycle) {
        (void)runCycle(&fixture);
    }
    while (offset < fixture.wire_length) {
        const uint8_t *frame = fixture.wire + offset;
        size_t length = KymoProtocol_FrameLength(frame[2]);
        if (frame[2] == KYMO_DESCRIPTOR_PARAM) {
            length = (size_t)frame[3] + 5u;
        } else {
            assert((frame[2] & KYMO_FLAG_TIMESTAMP) != 0u);
            assert(emb_read_u32_le(frame + TIMESTAMP_OFFSET) == sampledAt);
            seenPlot = (uint8_t)(seenPlot || frame[3] == PLOT_CHANNEL);
            seenParam = (uint8_t)(seenParam || frame[3] == PARAM_CHANNEL);
        }
        assert(length != 0u && length <= fixture.wire_length - offset);
        offset += length;
    }
    assert(seenPlot != 0u && seenParam != 0u);
}
#endif
/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    testControlSampleTime();
#if KYMO_EFFECTIVE_TIMESTAMP && KYMO_EFFECTIVE_RUNTIME && KYMO_EFFECTIVE_PARAMETERS
    testSharedDeviceClock(10000u, 12345u);
    testSharedDeviceClock(UINT32_MAX - 3u, 5u);
#endif
    testSetupErrors();
#if KYMO_EFFECTIVE_TRACE
    testMetadataRepeat(0u, 0u);
    testMetadataRepeat(20u, 0u);
    testMetadataRepeat(1u, 1u);
#endif
#if KYMO_EFFECTIVE_RUNTIME
    testPlotConfigurationAndRecovery();
#if KYMO_EFFECTIVE_WATCH
    testChannelCollision(0u);
    testChannelCollision(1u);
#endif
#endif
    testBackpressureAndFault();
#if KYMO_EFFECTIVE_PARAMETERS || KYMO_EFFECTIVE_WATCH
    testReadViolation();
#endif
#if KYMO_EFFECTIVE_WATCH
    testWatchChannelPolicy();
#endif
    testInvalidAndIndependent();
    testLifecycle(0u);
    testLifecycle(1u);
    return 0;
}
