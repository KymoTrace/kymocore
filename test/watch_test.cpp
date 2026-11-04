/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/* Behavior tests for live variables (KymoWatch_*): golden vectors, checks, sampling. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "KymoCore.h"

/* Simulated device memory at a fixed 32-bit base, read through the callback, so the
 * test behaves the same on 32- and 64-bit hosts. */
#define BASE 0x20000000u
static uint8_t sMemory[64];
static unsigned sReads = 0u;
static uint8_t sFailReads = 0u;

/*******************************************************************************
 * readMemory
 ******************************************************************************/
static uint8_t readMemory(void *context, uint32_t address, uint8_t size, uint8_t *output)
{
    uint8_t ok = 0u;
    (void)context;
    ++sReads;
    if (!sFailReads && (address >= BASE) && ((address - BASE) + size <= sizeof(sMemory))) {
        memcpy(output, sMemory + (address - BASE), size);
        ok = 1u;
    }
    return ok;
}

static const KymoWatchRange sRanges[] = { { BASE, 32u } };
static const uint8_t sBuildId[] = { 0xDEu, 0xADu, 0xBEu, 0xEFu };

/*******************************************************************************
 * initWatch
 ******************************************************************************/
static void initWatch(KymoWatch *watch)
{
    const KymoWatchConfig config = { sRanges, 1u, sBuildId, sizeof(sBuildId), readMemory, nullptr };
    assert(KymoWatch_Init(watch, &config) == KYMO_OK);
}

/*******************************************************************************
 * control
 ******************************************************************************/
/* Builds a control frame around payload; returns its length. */
static size_t control(uint8_t *frame, const uint8_t *payload, uint8_t length)
{
    frame[0] = KYMO_SYNC_0;
    frame[1] = KYMO_SYNC_1;
    frame[2] = KYMO_DESCRIPTOR_PARAM;
    frame[3] = length;
    memcpy(frame + 4, payload, length);
    frame[4u + length] = KymoProtocol_Crc8(frame + 2, 2u + length);
    return 5u + length;
}

/*******************************************************************************
 * set
 ******************************************************************************/
/* Sends one WATCH_SET and returns the acknowledged status. */
static uint8_t set(KymoWatch *watch, uint8_t slot, uint8_t channel, uint32_t address,
                   uint8_t type, uint16_t period)
{
    uint8_t frame[32];
    uint8_t reply[KYMO_WATCH_FRAME_MAX_SIZE];
    const uint8_t payload[] = { KYMO_WATCH_OP_SET, slot, channel,
                                (uint8_t)address, (uint8_t)(address >> 8), (uint8_t)(address >> 16),
                                (uint8_t)(address >> 24), type, (uint8_t)period, (uint8_t)(period >> 8) };
    assert(KymoWatch_Feed(watch, frame, control(frame, payload, sizeof(payload))) == 1u);
    const size_t length = KymoWatch_NextReply(watch, reply, sizeof(reply));
    assert((length == 8u) && (reply[4] == KYMO_WATCH_OP_ACK) && (reply[5] == slot));
    assert(reply[7] == KymoProtocol_Crc8(reply + 2, 5u));
    assert(KymoWatch_NextReply(watch, reply, sizeof(reply)) == 0u);
    return reply[6];
}

/*******************************************************************************
 * testInit
 ******************************************************************************/
static void testInit(void)
{
    KymoWatch watch;
    const KymoWatchRange empty[] = { { BASE, 0u } };
    const KymoWatchRange wrapping[] = { { 0xFFFFFFF0u, 0x20u } };
    const KymoWatchRange top[] = { { 0xFFFFFFF0u, 0x10u } };
    const uint8_t longId[KYMO_WATCH_BUILD_ID_MAX + 1u] = {};
    KymoWatchConfig config = { sRanges, 1u, nullptr, 0u, nullptr, nullptr };
    assert(KymoWatch_Init(&watch, &config) == KYMO_OK);
    assert(KymoWatch_Init(nullptr, &config) == KYMO_BAD_CONFIG);
    assert(KymoWatch_Init(&watch, nullptr) == KYMO_BAD_CONFIG);
    config.range_count = 0u;
    assert(KymoWatch_Init(&watch, &config) == KYMO_BAD_CONFIG);
    config.range_count = KYMO_WATCH_MAX_RANGES + 1u;
    assert(KymoWatch_Init(&watch, &config) == KYMO_BAD_CONFIG);
    config.range_count = 1u;
    config.ranges = empty;
    assert(KymoWatch_Init(&watch, &config) == KYMO_BAD_CONFIG);
    config.ranges = wrapping;
    assert(KymoWatch_Init(&watch, &config) == KYMO_BAD_CONFIG);
    config.ranges = top;
    assert(KymoWatch_Init(&watch, &config) == KYMO_OK);
    config.build_id = longId;
    config.build_id_length = sizeof(longId);
    assert(KymoWatch_Init(&watch, &config) == KYMO_BAD_CONFIG);
    config.build_id = nullptr;
    config.build_id_length = 4u;
    assert(KymoWatch_Init(&watch, &config) == KYMO_BAD_CONFIG);
    /* A failed Init leaves the watch unusable. */
    uint8_t frame[KYMO_WATCH_FRAME_MAX_SIZE];
    KymoDataPoint point;
    const uint8_t info[] = { 0xA5u, 0x5Au, 0x60u, 0x01u, 0x03u, 0xD9u };
    assert(KymoWatch_Feed(&watch, info, sizeof(info)) == 0u);
    assert(KymoWatch_NextReply(&watch, frame, sizeof(frame)) == 0u);
    assert(KymoWatch_NextSample(&watch, 0u, &point) == KYMO_BAD_CONFIG);
}

/*******************************************************************************
 * testGoldenVectors
 ******************************************************************************/
/* PROTOCOL.md, "Live variables": request, reply, SET and both ACK examples. */
static void testGoldenVectors(void)
{
    KymoWatch watch;
    uint8_t frame[KYMO_WATCH_FRAME_MAX_SIZE];
    const KymoWatchRange esp32[] = { { 0x3FFB0000u, 0x2C200u } };
    const KymoWatchConfig config = { esp32, 1u, sBuildId, sizeof(sBuildId), readMemory, nullptr };
    assert(KymoWatch_Init(&watch, &config) == KYMO_OK);
    const uint8_t info[] = { 0xA5u, 0x5Au, 0x60u, 0x01u, 0x03u, 0xD9u };
    assert(KymoWatch_Feed(&watch, info, sizeof(info)) == 1u);
    /* Too small a buffer keeps the reply due. */
    assert(KymoWatch_NextReply(&watch, frame, 10u) == 0u);
    const size_t length = KymoWatch_NextReply(&watch, frame, sizeof(frame));
    uint8_t reply[] = { 0xA5u, 0x5Au, 0x60u, 0x12u, 0x04u, 0x08u, 0x05u, 0x00u, 0x04u,
                        0xDEu, 0xADu, 0xBEu, 0xEFu, 0x01u, 0x00u, 0x00u, 0xFBu, 0x3Fu,
                        0x00u, 0xC2u, 0x02u, 0x00u, 0x75u };
    /* The vector assumes the default sizes; recompute only what a build changes. */
    reply[5] = (uint8_t)KYMO_MAX_WATCHES;
    reply[6] = (uint8_t)KYMO_WATCH_MIN_PERIOD_MS;
    reply[7] = (uint8_t)(KYMO_WATCH_MIN_PERIOD_MS >> 8);
    reply[sizeof(reply) - 1u] = KymoProtocol_Crc8(reply + 2, sizeof(reply) - 3u);
    assert((length == sizeof(reply)) && (memcmp(frame, reply, sizeof(reply)) == 0));
    if ((KYMO_MAX_WATCHES == 8u) && (KYMO_WATCH_MIN_PERIOD_MS == 5u)) {
        assert(frame[sizeof(reply) - 1u] == 0x75u);
    }
    assert(KymoWatch_NextReply(&watch, frame, sizeof(frame)) == 0u);

    const uint8_t setFrame[] = { 0xA5u, 0x5Au, 0x60u, 0x0Au, 0x05u, 0x00u, 0xC8u, 0x34u, 0x12u,
                                 0xFBu, 0x3Fu, 0x07u, 0x14u, 0x00u, 0x55u };
    assert(KymoWatch_Feed(&watch, setFrame, sizeof(setFrame)) == 1u);
    const uint8_t accepted[] = { 0xA5u, 0x5Au, 0x60u, 0x03u, 0x06u, 0x00u, 0x00u, 0xEBu };
    assert(KymoWatch_NextReply(&watch, frame, sizeof(frame)) == sizeof(accepted));
    assert(memcmp(frame, accepted, sizeof(accepted)) == 0);
    assert((watch.slots[0].channel == 200u) && (watch.slots[0].address == 0x3FFB1234u) &&
           (watch.slots[0].type == KYMO_WATCH_TYPE_F32) && (watch.slots[0].period_ms == 20u));

    /* Slot 1, address outside the range: a5 5a 60 03 06 01 02 f0. */
    const uint8_t outside[] = { KYMO_WATCH_OP_SET, 1u, 201u, 0x00u, 0x00u, 0x00u, 0x40u,
                                KYMO_WATCH_TYPE_U32, 20u, 0u };
    uint8_t request[32];
    assert(KymoWatch_Feed(&watch, request, control(request, outside, sizeof(outside))) == 1u);
    const uint8_t refused[] = { 0xA5u, 0x5Au, 0x60u, 0x03u, 0x06u, 0x01u, 0x02u, 0xF0u };
    assert(KymoWatch_NextReply(&watch, frame, sizeof(frame)) == sizeof(refused));
    assert(memcmp(frame, refused, sizeof(refused)) == 0);
    assert((watch.slots[1].type == 0u) && (watch.accepted == 1u) && (watch.rejected == 1u));
}

/*******************************************************************************
 * testChecks
 ******************************************************************************/
static void testChecks(void)
{
    KymoWatch watch;
    initWatch(&watch);
    assert(set(&watch, 0u, 200u, BASE, KYMO_WATCH_TYPE_U32, 10u) == KYMO_WATCH_STATUS_ACCEPTED);
    /* The last bytes of the range are fine, one byte further is not. */
    assert(set(&watch, 1u, 201u, BASE + 28u, KYMO_WATCH_TYPE_F32, 10u) == KYMO_WATCH_STATUS_ACCEPTED);
    assert(set(&watch, 1u, 201u, BASE + 32u, KYMO_WATCH_TYPE_U8, 10u) == KYMO_WATCH_STATUS_OUT_OF_RANGE);
    assert(set(&watch, 1u, 201u, BASE + 24u, KYMO_WATCH_TYPE_F64, 10u) == KYMO_WATCH_STATUS_ACCEPTED);
    assert(set(&watch, 1u, 201u, BASE + 32u, KYMO_WATCH_TYPE_F64, 10u) == KYMO_WATCH_STATUS_OUT_OF_RANGE);
    assert(set(&watch, 1u, 201u, BASE - 4u, KYMO_WATCH_TYPE_U32, 10u) == KYMO_WATCH_STATUS_OUT_OF_RANGE);
    assert(set(&watch, 1u, 201u, BASE + 2u, KYMO_WATCH_TYPE_U32, 10u) == KYMO_WATCH_STATUS_MISALIGNED);
    assert(set(&watch, 1u, 201u, BASE + 1u, KYMO_WATCH_TYPE_I16, 10u) == KYMO_WATCH_STATUS_MISALIGNED);
    assert(set(&watch, 1u, 201u, BASE + 1u, KYMO_WATCH_TYPE_U8, 10u) == KYMO_WATCH_STATUS_ACCEPTED);
    assert(set(&watch, 1u, 201u, BASE, 0u, 10u) == KYMO_WATCH_STATUS_BAD_TYPE);
    assert(set(&watch, 1u, 201u, BASE, 10u, 10u) == KYMO_WATCH_STATUS_BAD_TYPE);
#if KYMO_WATCH_MIN_PERIOD_MS > 1u
    /* With a 1 ms minimum the only shorter period is 0, which means "stop". */
    assert(set(&watch, 1u, 201u, BASE, KYMO_WATCH_TYPE_U8,
               (uint16_t)(KYMO_WATCH_MIN_PERIOD_MS - 1u)) == KYMO_WATCH_STATUS_BAD_PERIOD);
#endif
    assert(watch.slots[1].type == 0u);
    /* Period 0 stops a slot whatever the other fields say. */
    assert(set(&watch, 0u, 0u, 0u, 0u, 0u) == KYMO_WATCH_STATUS_ACCEPTED);
    assert(watch.slots[0].type == 0u);
    assert(set(&watch, (uint8_t)KYMO_MAX_WATCHES, 201u, BASE, KYMO_WATCH_TYPE_U8, 10u) ==
           KYMO_WATCH_STATUS_BAD_SLOT);
}

/*******************************************************************************
 * testDiscarded
 ******************************************************************************/
static void testDiscarded(void)
{
    KymoWatch watch;
    uint8_t frame[32];
    uint8_t reply[KYMO_WATCH_FRAME_MAX_SIZE];
    initWatch(&watch);
    const uint8_t info[] = { KYMO_WATCH_OP_INFO };
    const uint8_t infoLong[] = { KYMO_WATCH_OP_INFO, 0u };
    const uint8_t setShort[] = { KYMO_WATCH_OP_SET, 0u, 200u };
    const uint8_t unknown[] = { 0x7Fu };
    const uint8_t list[] = { 0x01u };  /* KymoParam's list request: not ours, not invalid */
    assert(KymoWatch_Feed(&watch, frame, control(frame, infoLong, sizeof(infoLong))) == 0u);
    assert(KymoWatch_Feed(&watch, frame, control(frame, setShort, sizeof(setShort))) == 0u);
    assert(KymoWatch_Feed(&watch, frame, control(frame, unknown, sizeof(unknown))) == 0u);
    assert(watch.invalid == 3u);
    assert(KymoWatch_Feed(&watch, frame, control(frame, list, sizeof(list))) == 0u);
    const size_t length = control(frame, info, sizeof(info));
    frame[length - 1u] = (uint8_t)(frame[length - 1u] ^ 0x01u);
    assert(KymoWatch_Feed(&watch, frame, length) == 0u);
    assert(watch.invalid == 4u);
    /* A DATA frame (a parameter write) is ignored, not counted. */
    uint8_t data[KYMO_FRAME_MAX_SIZE];
    KymoDataPoint point = {};
    point.id = 7u;
    point.value = 1.0f;
    assert(KymoProtocol_Encode(&point, data, sizeof(data)) == 9u);
    assert((KymoWatch_Feed(&watch, data, 9u) == 0u) && (watch.invalid == 4u));
    assert(KymoWatch_NextReply(&watch, reply, sizeof(reply)) == 0u);
    /* Split across reads and surrounded by noise. */
    const uint8_t noise[] = { 0x00u, 0xA5u, 0x13u };
    (void)KymoWatch_Feed(&watch, noise, sizeof(noise));
    const size_t infoLength = control(frame, info, sizeof(info));
    assert(KymoWatch_Feed(&watch, frame, 2u) == 0u);
    assert(KymoWatch_Feed(&watch, frame + 2, infoLength - 2u) == 1u);
    assert(KymoWatch_NextReply(&watch, reply, sizeof(reply)) != 0u);
    assert((KymoWatch_Feed(nullptr, frame, 1u) == 0u) && (KymoWatch_Feed(&watch, nullptr, 3u) == 0u));
    assert(KymoWatch_NextReply(nullptr, reply, sizeof(reply)) == 0u);
    assert(KymoWatch_NextReply(&watch, nullptr, sizeof(reply)) == 0u);
}

/*******************************************************************************
 * testInfoStopsSlots
 ******************************************************************************/
static void testInfoStopsSlots(void)
{
    KymoWatch watch;
    uint8_t frame[32];
    uint8_t reply[KYMO_WATCH_FRAME_MAX_SIZE];
    KymoDataPoint point;
    initWatch(&watch);
    assert(set(&watch, 0u, 200u, BASE, KYMO_WATCH_TYPE_U8, 10u) == KYMO_WATCH_STATUS_ACCEPTED);
    const uint8_t info[] = { KYMO_WATCH_OP_INFO };
    assert(KymoWatch_Feed(&watch, frame, control(frame, info, sizeof(info))) == 1u);
    assert(watch.slots[0].type == 0u);
    assert(KymoWatch_NextSample(&watch, 0u, &point) == KYMO_IDLE);
    assert(KymoWatch_NextReply(&watch, reply, sizeof(reply)) == 23u);
    assert(KymoWatch_NextReply(&watch, reply, sizeof(reply)) == 0u);
}

/*******************************************************************************
 * testSampling
 ******************************************************************************/
static void testSampling(void)
{
    KymoWatch watch;
    KymoDataPoint point;
    const float f32 = -2.5f;
    const double f64 = 1234.5;
    const int16_t i16 = -300;
    const uint32_t u32 = 70000u;
    memset(sMemory, 0, sizeof(sMemory));
    memcpy(sMemory + 0, &f32, sizeof(f32));
    memcpy(sMemory + 8, &f64, sizeof(f64));
    memcpy(sMemory + 4, &i16, sizeof(i16));
    memcpy(sMemory + 16, &u32, sizeof(u32));
    sMemory[20] = 0xFFu;  /* i8 -1, bool 1 */
    initWatch(&watch);
    assert(set(&watch, 0u, 200u, BASE + 0u, KYMO_WATCH_TYPE_F32, 10u) == 0u);
    assert(set(&watch, 1u, 201u, BASE + 8u, KYMO_WATCH_TYPE_F64, 20u) == 0u);
    assert(set(&watch, 2u, 202u, BASE + 4u, KYMO_WATCH_TYPE_I16, 10u) == 0u);
    assert(set(&watch, 3u, 203u, BASE + 16u, KYMO_WATCH_TYPE_U32, 10u) == 0u);
    assert(set(&watch, 4u, 204u, BASE + 20u, KYMO_WATCH_TYPE_I8, 10u) == 0u);
    assert(set(&watch, 5u, 205u, BASE + 20u, KYMO_WATCH_TYPE_BOOL, 10u) == 0u);
    /* Every new slot is sampled at once, one per call, round robin. */
    const float expected[] = { -2.5f, 1234.5f, -300.0f, 70000.0f, -1.0f, 1.0f };
    for (unsigned index = 0u; index < 6u; ++index) {
        assert(KymoWatch_NextSample(&watch, 1000u, &point) == KYMO_OK);
        assert((point.id == 200u + index) && (point.value == expected[index]) && (point.flags == 0u));
    }
    assert(KymoWatch_NextSample(&watch, 1009u, &point) == KYMO_IDLE);
    /* At 1010 the 10 ms slots are due, the 20 ms slot is not. */
    unsigned count = 0u;
    while (KymoWatch_NextSample(&watch, 1010u, &point) == KYMO_OK) {
        assert(point.id != 201u);
        ++count;
    }
    assert(count == 5u);
    /* After a long pause every slot fires once, not once per missed period. */
    count = 0u;
    while (KymoWatch_NextSample(&watch, 5000u, &point) == KYMO_OK) {
        ++count;
    }
    assert(count == 6u);
    assert(KymoWatch_NextSample(&watch, 5009u, &point) == KYMO_IDLE);
    /* The clock may wrap. */
    assert(set(&watch, 0u, 200u, BASE, KYMO_WATCH_TYPE_F32, 10u) == 0u);
    assert(KymoWatch_NextSample(&watch, 0xFFFFFFFCu, &point) == KYMO_OK);
    assert((point.id == 200u) && (KymoWatch_NextSample(&watch, 0xFFFFFFFDu, &point) == KYMO_IDLE));
    count = 0u;
    while (KymoWatch_NextSample(&watch, 6u, &point) == KYMO_OK) {
        count += (point.id == 200u) ? 1u : 0u;
    }
    assert(count == 1u);
    /* A failing reader skips the sample and counts it. */
    sFailReads = 1u;
    assert(KymoWatch_NextSample(&watch, 100000u, &point) == KYMO_IDLE);
    assert(watch.read_failures == 6u);
    sFailReads = 0u;
    assert(KymoWatch_NextSample(nullptr, 0u, &point) == KYMO_BAD_CONFIG);
    assert(KymoWatch_NextSample(&watch, 0u, nullptr) == KYMO_BAD_CONFIG);
}

/*******************************************************************************
 * testDirectRead
 ******************************************************************************/
/* Without a reader the variable is read in place; only where addresses fit 32 bits. */
static volatile float sDirect = 3.25f;

static void testDirectRead(void)
{
    if (sizeof(void *) == 4u) {
        KymoWatch watch;
        KymoDataPoint point;
        const uint32_t address = (uint32_t)(uintptr_t)&sDirect;
        const KymoWatchRange ranges[] = { { address, sizeof(sDirect) } };
        const KymoWatchConfig config = { ranges, 1u, nullptr, 0u, nullptr, nullptr };
        assert(KymoWatch_Init(&watch, &config) == KYMO_OK);
        assert(set(&watch, 0u, 210u, address, KYMO_WATCH_TYPE_F32, 10u) == 0u);
        assert((KymoWatch_NextSample(&watch, 0u, &point) == KYMO_OK) && (point.value == 3.25f));
        sDirect = 4.5f;
        assert((KymoWatch_NextSample(&watch, 10u, &point) == KYMO_OK) && (point.value == 4.5f));
    }
}

#if KYMO_EFFECTIVE_PARAMETERS
/*******************************************************************************
 * testSharedStream
 ******************************************************************************/
/* Parameters and live variables read the same bytes; neither counts the other's frames. */
static float sKp = 1.0f;

static void testSharedStream(void)
{
    KymoWatch watch;
    KymoParamReceiver receiver;
    const KymoParameter table[] = { KYMO_PARAM(7, sKp, 0.0f, 10.0f, "") };
    uint8_t stream[64];
    size_t length = 0u;
    initWatch(&watch);
    assert(KymoParam_Init(&receiver, table, 1u) == KYMO_OK);
    const uint8_t list[] = { 0xA5u, 0x5Au, 0x60u, 0x01u, 0x01u, 0xD7u };
    const uint8_t write[] = { 0xA5u, 0x5Au, 0x40u, 0x07u, 0x00u, 0x00u, 0x80u, 0x3Fu, 0x54u };
    const uint8_t set0[] = { KYMO_WATCH_OP_SET, 0u, 200u, 0x00u, 0x00u, 0x00u, 0x20u,
                             KYMO_WATCH_TYPE_U8, 10u, 0u };
    memcpy(stream, list, sizeof(list));
    length += sizeof(list);
    memcpy(stream + length, write, sizeof(write));
    length += sizeof(write);
    length += control(stream + length, set0, sizeof(set0));
    sKp = 3.0f;
    assert(KymoParam_Feed(&receiver, stream, length) == 1u);
    assert(KymoWatch_Feed(&watch, stream, length) == 1u);
    assert((sKp == 1.0f) && (receiver.invalid == 0u) && (watch.invalid == 0u));
    assert(watch.slots[0].type == KYMO_WATCH_TYPE_U8);
}
#endif

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    testInit();
    testGoldenVectors();
    testChecks();
    testDiscarded();
    testInfoStopsSlots();
    testSampling();
    testDirectRead();
#if KYMO_EFFECTIVE_PARAMETERS
    testSharedStream();
#endif
    printf("live variables: PASS\n");
    return 0;
}
