/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file quickstart.ino
 * @brief Plot two variables over Serial without writing a sample callback.
 * @details The sketch has two parts. The Arduino adapter (clock and bounded
 * Serial write) can stay as it is; replace it for other boards. The application
 * part binds two float variables to channels 0 and 1 with Kymo_SampleBound.
 * Runs with the default build settings: no flags, no device timestamps.
 */
#include <Arduino.h>
#include <KymoCore.h>

#if KYMO_EFFECTIVE_RUNTIME

/* ------------------------- Arduino adapter ------------------------------- */

/** @brief Arduino millisecond counter.
 * @param[in] user Unused; nullptr.
 * @return Current millis() tick.
 */
/*******************************************************************************
 * readClockMs
 ******************************************************************************/
static uint32_t readClockMs(void *user)
{
    uint32_t result = static_cast<uint32_t>(millis());
    (void)user;
    return result;
}

/** @brief Copy as many bytes as Serial can take right now.
 * @details Reporting fewer bytes than offered is fine; the runtime resumes
 * the rest on the next Kymo_Main() call without blocking loop().
 * @param[in] user Unused; nullptr.
 * @param[in] bytes Frame bytes; not retained.
 * @param[in] length Requested byte count.
 * @return Bytes accepted by Serial, zero when its buffer is full.
 */
/*******************************************************************************
 * writeBytes
 ******************************************************************************/
static uint8_t writeBytes(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t result = 0U;
    int available = Serial.availableForWrite();
    (void)user;
    if ((bytes != nullptr) && (available > 0))
    {
        if (available < length)
        {
            length = static_cast<uint8_t>(available);
        }
        result = static_cast<uint8_t>(Serial.write(bytes, length));
    }
    return result;
}

/* ------------------------- Application ----------------------------------- */

/* The variables to plot: raw ADC readings of A0 and A1. */
static float adc0 = 0.0f;
static float adc1 = 0.0f;

/* period_ms, id, flags: channels 0 and 1, every 20 ms, scalar value only. */
static const KymoChannel channels[] = {{20U, 0U, 0U}, {20U, 1U, 0U}};
/* One variable per channel, in the same order as the channel table. */
static const float *const values[] = {&adc0, &adc1};
static_assert(KYMO_COUNT_OF(values) == KYMO_COUNT_OF(channels), "one variable per channel");

static KymoPlotBinding binding = {channels, values, KYMO_COUNT_OF(channels)};
static KymoContext context = {};
static KymoStatus initStatus = KYMO_BAD_CONFIG;

/*******************************************************************************
 * setup
 ******************************************************************************/
void setup()
{
    KymoConfig config = {}; /* copied by Kymo_Init; may be local */
    Serial.begin(115200);
    config.channels = channels;
    config.channel_count = KYMO_COUNT_OF(channels);
    config.clock_ms = readClockMs;
    config.sample = Kymo_SampleBound; /* reads the bound variables */
    config.sample_user = &binding;
    config.write = writeBytes;
    initStatus = Kymo_Init(&context, &config);
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop()
{
    adc0 = static_cast<float>(analogRead(A0));
    adc1 = static_cast<float>(analogRead(A1));
    if (initStatus == KYMO_OK)
    {
        (void)Kymo_Main(&context);
    }
}

#else

/*******************************************************************************
 * setup
 ******************************************************************************/
void setup()
{
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop()
{
}

#endif
