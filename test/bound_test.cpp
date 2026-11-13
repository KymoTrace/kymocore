/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/* Variable binding: Kymo_SampleBound plots float variables through Init/Main. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "KymoCore.h"
#include "common/embedded_bytes.h"

static const uint8_t ID_FAST = 3u;
static const uint8_t ID_SLOW = 9u;
static const uint8_t ID_UNBOUND = 4u;
static const size_t FRAME_ID_OFFSET = 3u;
static const size_t FRAME_VALUE_OFFSET = 4u;

static uint32_t sNowMs = 1000u;
static uint8_t sFrame[KYMO_FRAME_MAX_SIZE];
static uint8_t sFrameLength = 0u;

static float sFast = 1.5f;
static float sSlow = -2.25f;
/* period_ms, id, flags: deliberately non-consecutive IDs. */
static const KymoChannel CHANNELS[] = {{10u, ID_FAST, 0u}, {30u, ID_SLOW, 0u}};
static const float *const VALUES[] = {&sFast, &sSlow};
static KymoPlotBinding sBinding = {CHANNELS, VALUES, KYMO_COUNT_OF(CHANNELS)};
static KymoContext sContext;

/*******************************************************************************
 * readClock
 ******************************************************************************/
static uint32_t readClock(void *user)
{
    uint32_t now = sNowMs;
    (void)user;
    return now;
}

/*******************************************************************************
 * writeFrame
 ******************************************************************************/
static uint8_t writeFrame(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t accepted = length;
    (void)user;
    assert(length <= sizeof(sFrame));
    memcpy(sFrame, bytes, length);
    sFrameLength = length;
    return accepted;
}

/*******************************************************************************
 * expectFrame
 ******************************************************************************/
static void expectFrame(uint8_t id, float value)
{
    float sent = 0.0f;
    sFrameLength = 0u;
    assert(Kymo_Main(&sContext) == KYMO_OK);
    assert(sFrameLength > (FRAME_VALUE_OFFSET + sizeof(sent)));
    assert(sFrame[FRAME_ID_OFFSET] == id);
    sent = emb_read_f32_le(sFrame + FRAME_VALUE_OFFSET);
    assert(sent == value);
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main()
{
    int result = 0;
    KymoSample sample = {};
    KymoPlotBinding hole = {CHANNELS, nullptr, KYMO_COUNT_OF(CHANNELS)};
    KymoConfig config = {};
    config.channels = CHANNELS;
    config.channel_count = KYMO_COUNT_OF(CHANNELS);
    config.clock_ms = readClock;
    config.sample = Kymo_SampleBound;
    config.sample_user = &sBinding;
    config.write = writeFrame;
    assert(Kymo_Init(&sContext, &config) == KYMO_OK);

    /* Both channels are due at start; each ID carries its own variable. */
    expectFrame(ID_FAST, 1.5f);
    expectFrame(ID_SLOW, -2.25f);
    assert(Kymo_Main(&sContext) == KYMO_IDLE);

    /* A changed variable appears at the next due sample; periods are kept. */
    sFast = 4.0f;
    sSlow = 8.0f;
    sNowMs += 10u;
    expectFrame(ID_FAST, 4.0f);
    assert(Kymo_Main(&sContext) == KYMO_IDLE);
    sNowMs += 20u;
    /* Round robin continues behind the last sent channel. */
    expectFrame(ID_SLOW, 8.0f);
    expectFrame(ID_FAST, 4.0f);

    /* Unbound IDs and missing tables are skipped instead of read. */
    assert(Kymo_SampleBound(&sBinding, ID_UNBOUND, &sample) == 0u);
    assert(Kymo_SampleBound(nullptr, ID_FAST, &sample) == 0u);
    assert(Kymo_SampleBound(&sBinding, ID_FAST, nullptr) == 0u);
    assert(Kymo_SampleBound(&hole, ID_FAST, &sample) == 0u);

    /* Enabled X/Z channels must be skipped without emitting zero coordinates. */
#if KYMO_EFFECTIVE_X && KYMO_EFFECTIVE_Z && KYMO_EFFECTIVE_TIMESTAMP
    const KymoChannel scalarChannels[] = {
        {10u, ID_FAST, KYMO_FLAG_X},
        {10u, ID_SLOW, KYMO_FLAG_Z},
        {10u, ID_UNBOUND, KYMO_FLAG_TIMESTAMP}};
    const float *const scalarValues[] = {&sFast, &sSlow, &sFast};
    KymoPlotBinding scalarBinding = {scalarChannels, scalarValues, KYMO_COUNT_OF(scalarChannels)};
    config.channels = scalarChannels;
    config.channel_count = KYMO_COUNT_OF(scalarChannels);
    config.sample_user = &scalarBinding;
    assert(Kymo_Init(&sContext, &config) == KYMO_OK);
    sFrameLength = 0u;
    assert(Kymo_Main(&sContext) == KYMO_SKIPPED);
    assert(Kymo_Main(&sContext) == KYMO_SKIPPED);
    assert(sFrameLength == 0u);
    expectFrame(ID_UNBOUND, sFast);
#endif

    printf("bound_test: PASS\n");
    return result;
}
