/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file main.cpp
 * @brief Portable Init/Main example with a simulated clock and copying transport.
 * @details Produces ten scalar frames without a board, SDK, heap or decoder.
 * The application owns every buffer. Replace the callbacks for actual hardware.
 */
#include <KymoCore.h>

#if KYMO_EFFECTIVE_RUNTIME

static uint32_t sTickMs = 0U;
static uint8_t sCapturedFrame[KYMO_FRAME_MAX_SIZE] = {};
static uint8_t sCapturedLength = 0U;
static KymoContext sContext = {};

/* period_ms, id, flags: channel 7, every 20 ms, scalar value only. */
static const KymoChannel sChannels[] = {{20U, 7U, 0U}};

/** @brief Read the simulated monotonic clock.
 * @param[in] pUser Unused; pass nullptr.
 * @return Simulated milliseconds since program start.
 */
/*******************************************************************************
 * readClockMs
 ******************************************************************************/
static uint32_t readClockMs(void *pUser)
{
    uint32_t result = sTickMs;
    (void)pUser;
    return result;
}

/** @brief Supply a scalar value for channel seven.
 * @param[in] pUser Unused; pass nullptr.
 * @param[in] channelId Configured channel identifier.
 * @param[out] pSample Destination; null is rejected.
 * @return One for a valid sample, otherwise zero.
 */
/*******************************************************************************
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *pUser, uint8_t channelId, KymoSample *pSample)
{
    uint8_t result = 0U;
    (void)pUser;
    if ((pSample != nullptr) && (channelId == 7U))
    {
        pSample->value = 1.0f;
        result = 1U;
    }
    return result;
}

/** @brief Copy a complete frame into application-owned capture storage.
 * @details The pointer is never retained; no busy callback is necessary.
 * @param[in] pUser Unused; pass nullptr.
 * @param[in] pBytes Readable frame bytes; null is rejected.
 * @param[in] length Requested bytes; must fit the capture buffer.
 * @return Number of bytes copied, or zero if rejected.
 */
/*******************************************************************************
 * writeFrame
 ******************************************************************************/
static uint8_t writeFrame(void *pUser, const uint8_t *pBytes, uint8_t length)
{
    uint8_t result = 0U;
    (void)pUser;
    if ((pBytes != nullptr) && (length <= sizeof(sCapturedFrame)))
    {
        for (uint8_t index = 0U; index < length; ++index)
        {
            sCapturedFrame[index] = pBytes[index];
        }
        sCapturedLength = length;
        result = length;
    }
    return result;
}

/** @brief Run ten bounded sampling steps and check a captured scalar frame.
 * @return Zero on success, one on initialization, scheduling or framing failure.
 */
/*******************************************************************************
 * main
 ******************************************************************************/
int main()
{
    int result = 0;
    KymoStatus status = KYMO_BAD_CONFIG;
    KymoConfig config = {}; /* copied by Kymo_Init; may be local */
    config.channels = sChannels;
    config.channel_count = KYMO_COUNT_OF(sChannels);
    config.clock_ms = readClockMs;
    config.sample = readSample;
    config.write = writeFrame;
    status = Kymo_Init(&sContext, &config);
    if (status != KYMO_OK)
    {
        result = 1;
    }
    for (uint8_t step = 0U; (step < 10U) && (result == 0); ++step)
    {
        status = Kymo_Main(&sContext);
        if ((status != KYMO_OK) || (sCapturedLength != 9U) || (sCapturedFrame[0] != KYMO_SYNC_0) ||
            (sCapturedFrame[1] != KYMO_SYNC_1) || (sCapturedFrame[3] != 7U))
        {
            result = 1;
        }
        sTickMs += 20U;
    }
    return result;
}

#else

/*******************************************************************************
 * main
 ******************************************************************************/
int main()
{
    int result = 0;
    return result;
}

#endif
