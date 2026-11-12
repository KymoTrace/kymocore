/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file multi_sensor_example.ino
 * @brief Independently scheduled Y, XY and XYZ channels over Serial.
 * @details Same four steps as simple_analog_example (WHAT, WHEN, VALUE,
 * WHERE); only the channel table and the sample callback differ. The flags of
 * each channel decide which coordinates reach the wire, so one callback can
 * always fill x/value/z. Requires KYMO_ENABLE_X=1, KYMO_ENABLE_Z=1 and
 * KYMO_ENABLE_TIMESTAMP=1 (set by platformio.ini).
 */
#include <Arduino.h>
#include <math.h>
#include <KymoCore.h>

#if KYMO_ENABLED && (!KYMO_ENABLE_X || !KYMO_ENABLE_Z || !KYMO_ENABLE_TIMESTAMP)
#error "This example requires KYMO_ENABLE_X=1, KYMO_ENABLE_Z=1 and KYMO_ENABLE_TIMESTAMP=1"
#endif

#if KYMO_EFFECTIVE_RUNTIME

/* 1. WHAT: three channels with their own period and coordinate set. */
static const KymoChannel channels[] = {
    /* period_ms, id, flags */
    {20U, 0U, KYMO_FLAG_TIMESTAMP},                         /* Y   */
    {50U, 1U, EMB_U8_OR(KYMO_FLAG_X, KYMO_FLAG_TIMESTAMP)}, /* XY  */
    {100U, 2U, KYMO_ALLOWED_FLAGS},                         /* XYZ */
};

/** @brief Runtime state: schedule and the single transmit buffer. */
static KymoContext context = {};

/** @brief 2. WHEN: Arduino millisecond counter.
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

/** @brief 3. VALUE: phase as X, sine/sawtooth as Y and cosine as Z.
 * @param[in] user Unused; nullptr.
 * @param[in] id Channel ID from the table.
 * @param[out] out Zero-initialized sample to fill.
 * @return One when a sample was written.
 */
/*******************************************************************************
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *user, uint8_t id, KymoSample *out)
{
    uint8_t result = 0U;
    const float phase = static_cast<float>(millis() % 2000UL) / 2000.0f;
    (void)user;
    if (out != nullptr)
    {
        out->x = phase;
        if (id == 0U)
        {
            out->value = sinf(phase * 6.283185307f);
        }
        else
        {
            out->value = (2.0f * phase) - 1.0f;
        }
        out->z = cosf(phase * 6.283185307f);
        result = 1U;
    }
    return result;
}

/** @brief 4. WHERE: copy as many bytes as Serial can take right now.
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
    config.sample = readSample;
    config.write = writeBytes;
    (void)Kymo_Init(&context, &config);
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop()
{
    (void)Kymo_Main(&context);
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
