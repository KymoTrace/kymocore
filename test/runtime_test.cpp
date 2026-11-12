/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/* Behavior tests for the cyclic telemetry runtime (Kymo_Init / Kymo_Main). */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "KymoCore.h"

static uint32_t sNowMs = 1000u;
static uint8_t sWriteLimit = 255u;
static uint8_t sOverAccept = 0u;
static uint8_t sBusy = 0u;
static uint8_t sSkip = 0u;
static unsigned sServiceCalls = 0u;
static uint8_t sBytes[512];
static size_t sByteCount = 0u;

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
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *user, uint8_t id, KymoSample *sample)
{
    uint8_t result = 0u;
    float *values = static_cast<float *>(user);
    if (sSkip == 0u)
    {
        sample->value = (values != nullptr) ? values[id] : (float)id;
        result = 1u;
    }
    return result;
}

/*******************************************************************************
 * writeBytes
 ******************************************************************************/
static uint8_t writeBytes(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t accepted = (length < sWriteLimit) ? length : sWriteLimit;
    (void)user;
    if (sOverAccept != 0u)
    {
        accepted = (uint8_t)(length + 1u);
    }
    else
    {
        assert(sByteCount + accepted <= sizeof(sBytes));
        memcpy(sBytes + sByteCount, bytes, accepted);
        sByteCount += accepted;
    }
    return accepted;
}

/*******************************************************************************
 * isBusy
 ******************************************************************************/
static uint8_t isBusy(void *user)
{
    uint8_t busy = sBusy;
    (void)user;
    return busy;
}

/*******************************************************************************
 * service
 ******************************************************************************/
static void service(void *user)
{
    (void)user;
    ++sServiceCalls;
}

static const KymoChannel sChannels[] = {
    {10u, 0u, 0u},
    {30u, 1u, 0u},
};

/*******************************************************************************
 * initFromTemporary
 ******************************************************************************/
/* The configuration lives on this stack frame only: Init must copy it. */
static KymoStatus initFromTemporary(KymoContext *context, float *values)
{
    KymoConfig config = {};
    config.channels = sChannels;
    config.channel_count = KYMO_COUNT_OF(sChannels);
    config.clock_ms = readClock;
    config.sample = readSample;
    config.write = writeBytes;
    config.busy = isBusy;
    config.service = service;
    config.sample_user = values;
    KymoStatus status = Kymo_Init(context, &config);
    memset(&config, 0xA5, sizeof(config));
    return status;
}

/*******************************************************************************
 * testValidation
 ******************************************************************************/
static void testValidation(void)
{
    KymoContext context = {};
    KymoConfig config = {};
    static const KymoChannel duplicate[] = {{10u, 3u, 0u}, {20u, 3u, 0u}};
    static KymoChannel tooMany[KYMO_MAX_CHANNELS + 1u];
    assert(Kymo_Init(nullptr, &config) == KYMO_BAD_CONFIG);
    assert(Kymo_Init(&context, nullptr) == KYMO_BAD_CONFIG);
    assert(Kymo_Init(&context, &config) == KYMO_BAD_CONFIG);
    config.channels = sChannels;
    config.channel_count = KYMO_COUNT_OF(sChannels);
    config.clock_ms = readClock;
    config.sample = readSample;
    assert(Kymo_Init(&context, &config) == KYMO_BAD_CONFIG); /* write missing */
    config.write = writeBytes;
    assert(Kymo_Init(&context, &config) == KYMO_OK);
    config.channels = duplicate;
    config.channel_count = KYMO_COUNT_OF(duplicate);
    assert(Kymo_Init(&context, &config) == KYMO_BAD_CONFIG);
    assert(Kymo_Main(&context) == KYMO_BAD_CONFIG); /* failed Init disables */
    for (uint16_t i = 0u; i < KYMO_COUNT_OF(tooMany); ++i)
    {
        tooMany[i].id = (uint8_t)i;
    }
    config.channels = tooMany;
    config.channel_count = KYMO_COUNT_OF(tooMany);
    assert(Kymo_Init(&context, &config) == KYMO_BAD_CONFIG);
    config.channel_count = KYMO_MAX_CHANNELS;
    assert(Kymo_Init(&context, &config) == KYMO_OK);
    config.channels = sChannels;
    config.channel_count = 1u;
    KymoChannel unsupported = {10u, 9u, KYMO_FLAG_X};
    config.channels = &unsupported;
    assert(Kymo_Init(&context, &config) == (KYMO_ENABLE_X ? KYMO_OK : KYMO_BAD_CONFIG));
}

/*******************************************************************************
 * testScheduling
 ******************************************************************************/
static void testScheduling(void)
{
    KymoContext context = {};
    float values[2] = {1.5f, -2.0f};
    sByteCount = 0u;
    sServiceCalls = 0u;
    assert(initFromTemporary(&context, values) == KYMO_OK);
    /* Both channels are due immediately; round robin serves one per call. */
    assert(Kymo_Main(&context) == KYMO_OK);
    assert(Kymo_Main(&context) == KYMO_OK);
    assert(Kymo_Main(&context) == KYMO_IDLE);
    assert(sServiceCalls == 3u);
    size_t frameSize = sByteCount / 2u;
    KymoDataPoint point = {};
#if KYMO_ENABLE_DECODER
    assert(KymoProtocol_Decode(sBytes, frameSize, &point) == KYMO_OK);
    assert((point.id == 0u) && (point.value == 1.5f));
    assert(KymoProtocol_Decode(sBytes + frameSize, frameSize, &point) == KYMO_OK);
    assert((point.id == 1u) && (point.value == -2.0f));
#else
    (void)point;
    assert(sBytes[3] == 0u);
    assert(sBytes[frameSize + 3u] == 1u);
#endif
    sNowMs += 10u;
    assert(Kymo_Main(&context) == KYMO_OK);   /* channel 0 again */
    assert(Kymo_Main(&context) == KYMO_IDLE); /* channel 1 waits for 30 ms */
    sNowMs += 20u;
    assert(Kymo_Main(&context) == KYMO_OK);
    assert(Kymo_Main(&context) == KYMO_OK);
    assert(sByteCount == 5u * frameSize);

    /* Short writes resume at the remaining offset without resampling. */
    sByteCount = 0u;
    sWriteLimit = 2u;
    sNowMs += 10u;
    KymoStatus status = Kymo_Main(&context);
    unsigned calls = 1u;
    while (status == KYMO_BUSY)
    {
        status = Kymo_Main(&context);
        ++calls;
    }
    assert(status == KYMO_OK);
    assert(sByteCount == frameSize);
    assert(calls == (frameSize + 1u) / 2u);
    sWriteLimit = 255u;

    /* Busy transport blocks sampling; service still runs. */
    sBusy = 1u;
    sNowMs += 10u;
    unsigned before = sServiceCalls;
    assert(Kymo_Main(&context) == KYMO_BUSY);
    assert(sServiceCalls == before + 1u);
    sBusy = 0u;

    /* A skipped sample advances the schedule without output. */
    sSkip = 1u;
    sByteCount = 0u;
    assert(Kymo_Main(&context) == KYMO_SKIPPED);
    assert(sByteCount == 0u);
    sSkip = 0u;

    /* Over-reporting drivers latch a fault until reinitialization. */
    sOverAccept = 1u;
    sNowMs += 100u;
    assert(Kymo_Main(&context) == KYMO_IO_ERROR);
    sOverAccept = 0u;
    assert(Kymo_Main(&context) == KYMO_IO_ERROR);
    assert(initFromTemporary(&context, values) == KYMO_OK);
    assert(Kymo_Main(&context) == KYMO_OK);
}

/*******************************************************************************
 * testNonFiniteSample
 ******************************************************************************/
static void testNonFiniteSample(void)
{
    KymoContext context = {};
    float values[2] = {0.0f, 0.0f};
    values[0] = values[0] / values[0];
    assert(initFromTemporary(&context, values) == KYMO_OK);
    assert(Kymo_Main(&context) == KYMO_BAD_SAMPLE);
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main()
{
    int result = 0;
    testValidation();
    testScheduling();
    testNonFiniteSample();
    puts("Telemetry runtime behavior passed");
    return result;
}
