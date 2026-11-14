/* SPDX-License-Identifier: MIT */
/** @file kymo_plot_types.h
 * @brief Canonical application channel, sample and binding types; no runtime state.
 */
#ifndef KYMO_PLOT_TYPES_H
#define KYMO_PLOT_TYPES_H
#include "../../../kymo_types.h"
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Immutable scheduling and wire selection for one channel.
     *
     * @details Input configuration. Channel IDs must be unique within one KymoConfig.
     * The structure is never transmitted directly.
     * @par Usage
     * static const KymoChannel channel = {20u, 0u, KYMO_FLAG_TIMESTAMP};
     */
    typedef struct
    {
        /**
         * @brief Requested interval in milliseconds, 0..INT32_MAX.
         *
         * @details Zero means every eligible Main call. Backpressure may delay
         * sampling; missed samples are not replayed.
         * @par Usage
         * Use 20 for a nominal 50 Hz channel.
         */
        uint32_t periodMs;

        /**
         * @brief Application channel ID, 0..255.
         *
         * @details Passed to the sample callback and encoded into the frame.
         * @par Usage
         * Map this ID to a sensor or application parameter in the sample callback.
         */
        uint8_t id;

        /**
         * @brief Optional wire fields for this channel.
         *
         * @details Only KYMO_SUPPORTED_FLAGS bits may be set in this build;
         * disabled fields make KymoRuntime_init return KYMO_BAD_CONFIG.
         * @par Usage
         * Use zero for scalar Y, or KYMO_ALLOWED_FLAGS for full XYZ/time.
         */
        uint8_t flags;
    } KymoChannel;

    /**
     * @brief Coordinates written by the application sample callback.
     *
     * @details Main zero-initializes this object for each attempt. Channel ID and
     * timestamp are supplied by the runtime.
     * @par Usage
     * sample->value = sensor_value; then return nonzero from the callback.
     */
    typedef struct
    {
        /**
         * @brief Optional X coordinate; must be finite if the channel enables X.
         * @par Usage
         * Assign position, angle or an independent-variable coordinate.
         */
        float x;

        /**
         * @brief Required scalar/Y measurement; must be finite.
         * @par Usage
         * Assign the current application measurement.
         */
        float value;

        /**
         * @brief Optional Z coordinate; must be finite if the channel enables Z.
         * @par Usage
         * Assign the third coordinate for XYZ telemetry.
         */
        float z;
    } KymoSample;

    /**
     * @brief Obtain a current measurement for a selected channel.
     *
     * @details Called at most once per Main. The sample pointer is valid only for this call.
     * @param[in,out] pContext pSampleContext from the configuration; may be NULL.
     * @param[in] channelId Configured channel ID selecting the source measurement.
     * @param[out] pSample Non-null, zero-initialized destination.
     * @return Nonzero if a measurement is available; zero to skip until the next period.
     * @par Usage
     * Switch on id, copy values into sample, then return one.
     */
    typedef uint8_t (*KymoSampleFn)(void *pContext, uint8_t channelId, KymoSample *pSample);

    /**
     * @brief Bind scalar plot channels to float variables instead of a sample callback.
     *
     * @details Input for KymoRuntime_sampleBound. values[i] is the variable plotted on
     * channels[i]; both tables therefore have the same order and count entries.
     * The tables and variables must outlive the runtime. A swapped order cannot be
     * detected and is part of the caller contract. No state, no heap.
     * @par Usage
     * @code{.c}
     * static float adc0, adc1;
     * static const KymoChannel channels[] = {{20u, 0u, 0u}, {20u, 1u, 0u}};
     * static const float *const values[] = {&adc0, &adc1};
     * static KymoPlotBinding binding = {channels, values, KYMO_COUNT_OF(channels)};
     * // config.sample = KymoRuntime_sampleBound; config.sample_user = &binding;
     * @endcode
     */
    typedef struct
    {
        /** @brief Channel table, the same one passed as KymoConfig.channels. */
        const KymoChannel *pChannelsAv;

        /** @brief One readable float per channel, in channel table order. */
        const float *const *pValuesAv;

        /** @brief Number of entries in both tables. */
        uint16_t channelCount;
    } KymoPlotBinding;
#ifdef __cplusplus
}
#endif
#endif
