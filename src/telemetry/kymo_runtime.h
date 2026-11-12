/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_runtime.h
 * @brief Cyclic telemetry runtime: describe channels once, call Kymo_Main forever.
 *
 * @details The application answers four questions and KymoCore does the rest:
 * - WHAT:  a channel table (ID, period, optional X/Z/timestamp fields),
 * - WHEN:  a millisecond clock callback,
 * - VALUE: a sample callback that fills one KymoSample per channel ID,
 * - WHERE: a write callback that hands frame bytes to any transport.
 *
 * Kymo_Init validates and copies the configuration into the context; only the
 * channel table must stay alive. Kymo_Main performs one bounded step: it
 * resumes a partial write or samples at most one due channel. No heap, no
 * blocking, no SDK types. Available with KYMO_ENABLE_RUNTIME=1 (default).
 * @defgroup kymo_runtime Telemetry runtime
 * @{
 * @par Usage
 * @code{.c}
 * static const KymoChannel channels[] = {{20u, 0u, 0u}};  // period_ms, id, flags
 * static KymoContext context;
 * void setup(void) {
 *     KymoConfig config = {0};
 *     config.channels = channels;
 *     config.channel_count = KYMO_COUNT_OF(channels);
 *     config.clock_ms = read_clock_ms;
 *     config.sample = read_sample;
 *     config.write = write_bytes;
 *     (void)Kymo_Init(&context, &config);
 * }
 * void loop(void) { (void)Kymo_Main(&context); }
 * @endcode
 */
#ifndef KYMO_RUNTIME_H
#define KYMO_RUNTIME_H

#include "../kymo_types.h"
#include "kymo_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @name Channel and sample data
     * @{ */
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
        uint32_t period_ms;
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
         * disabled fields make Kymo_Init return KYMO_BAD_CONFIG.
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
     * @param[in,out] user sample_user from the configuration; may be NULL.
     * @param[in] id Configured channel ID selecting the source measurement.
     * @param[out] sample Non-null, zero-initialized destination.
     * @return Nonzero if a measurement is available; zero to skip until the next period.
     * @par Usage
     * Switch on id, copy values into sample, then return one.
     */
    typedef uint8_t (*KymoSampleFn)(void *user, uint8_t id, KymoSample *sample);
    /** @} */

    /** @name Configuration and runtime storage
     * @{ */
    /**
     * @brief Runtime wiring: required fields first, optional fields may stay zero.
     *
     * @details Input to Kymo_Init, which validates and copies it into the context.
     * The configuration object itself may therefore be a local variable; the channel
     * table and every user context must stay alive while the runtime is used.
     * Fill the fields by name starting from a zero-initialized object.
     * @par Usage
     * KymoConfig config = {0}; config.channels = channels; ... Kymo_Init(&context, &config);
     */
    typedef struct
    {
        /**
         * @brief Required: channel table with channel_count entries.
         * @details Must outlive the runtime and remain unchanged.
         * @par Usage
         * Point to a static const KymoChannel table.
         */
        const KymoChannel *channels;
        /**
         * @brief Required: number of channels, 1..KYMO_MAX_CHANNELS.
         * @par Usage
         * config.channel_count = KYMO_COUNT_OF(channels);
         */
        uint16_t channel_count;
        /**
         * @brief Required: monotonic millisecond clock.
         * @par Usage
         * Assign a platform wrapper returning uint32 milliseconds.
         */
        KymoClockFn clock_ms;
        /**
         * @brief Required: callback producing channel coordinates.
         * @par Usage
         * Assign the application sensor/parameter reader.
         */
        KymoSampleFn sample;
        /**
         * @brief Required: binary transport callback.
         * @par Usage
         * Bind UART, USB, TCP or a KymoMuxDataSlot in the application.
         */
        KymoWriteFn write;
        /**
         * @brief Optional: buffer-ownership callback; NULL means write copies the data.
         * @details Drivers that keep the byte pointer after write returns must provide it.
         * @par Usage
         * Bind an asynchronous driver completion check.
         */
        KymoBusyFn busy;
        /**
         * @brief Optional: transport maintenance called once per Main.
         * @details Called even while busy or faulted. NULL disables maintenance.
         * @par Usage
         * Bind a bounded network/driver poll function.
         */
        KymoServiceFn service;
        /**
         * @brief Optional: context forwarded to clock_ms.
         * @par Usage
         * Leave NULL for a global platform clock.
         */
        void *clock_user;
        /**
         * @brief Optional: context forwarded to sample.
         * @par Usage
         * Point to the application parameter structure instead of using globals.
         */
        void *sample_user;
        /**
         * @brief Optional: context shared by write, busy and service.
         * @par Usage
         * Point to a driver handle wrapper.
         */
        void *transport_user;
    } KymoConfig;

    /**
     * @brief Caller-owned runtime state including the single transmit buffer.
     *
     * @details One instance per independently driven transport. Fields are exposed
     * only for static allocation; applications must not edit them. Size grows with
     * KYMO_MAX_CHANNELS (four bytes per channel). Zero initialization is safe.
     * @par Usage
     * static KymoContext context; then Kymo_Init(&context, &config).
     */
    typedef struct
    {
        /** @brief Validated copy of the configuration passed to Kymo_Init. */
        KymoConfig config;
        /** @brief Last sample tick per channel; index matches config.channels. */
        uint32_t last_sample_ms[KYMO_MAX_CHANNELS];
        /** @brief Clock epoch captured by Init for relative timestamps. */
        uint32_t start_ms;
        /** @brief Round-robin index of the next channel to inspect. */
        uint16_t next_channel;
        /** @brief Fixed frame storage; must stay unchanged while the transport is busy. */
        uint8_t tx[KYMO_FRAME_MAX_SIZE];
        /** @brief Encoded frame length, zero or 9..21 bytes. */
        uint8_t tx_length;
        /** @brief Frame bytes already accepted by the driver. */
        uint8_t tx_offset;
        /** @brief Latched driver-contract failure flag; see KYMO_IO_ERROR. */
        uint8_t fault;
        /** @brief Nonzero after a successful Kymo_Init. */
        uint8_t initialized;
    } KymoContext;
    /** @} */

#if KYMO_EFFECTIVE_RUNTIME || defined(DOXYGEN)
    /** @name Lifecycle
     * @{ */
    /**
     * @brief Validate and copy a configuration and enable a runtime instance.
     *
     * @details Checks required callbacks, channel count, unique IDs, build-supported
     * flags and periods. Valid channels become immediately due. The context is cleared
     * first, so failure leaves it disabled. No heap allocation or peripheral setup.
     * @param[out] context Writable, persistent runtime storage; NULL is rejected.
     * @param[in] config Configuration to copy; may be a temporary object.
     * @return KYMO_OK on success, KYMO_BAD_CONFIG on validation failure.
     * @pre Stop any driver borrowing context->tx before reinitialization.
     * @par Usage
     * Initialize hardware, then check Kymo_Init(&context, &config) == KYMO_OK.
     */
    KymoStatus Kymo_Init(KymoContext *context, const KymoConfig *config);

    /**
     * @brief Perform one bounded scheduling/transmission step.
     *
     * @details Services the driver, respects a latched fault or busy state, resumes
     * pending bytes or samples one due channel. At most one sample and one write
     * occur, with at most channel_count checks. This is not an ISR API.
     * @param[in,out] context Initialized persistent state.
     * @return KYMO_OK, KYMO_IDLE, KYMO_BUSY, KYMO_SKIPPED, KYMO_BAD_SAMPLE,
     *         KYMO_BAD_CONFIG or KYMO_IO_ERROR.
     * @pre Do not call concurrently or recursively; call at least once within INT32_MAX ms.
     * @par Usage
     * Call from a cyclic task faster than the sum of channel rates.
     */
    KymoStatus Kymo_Main(KymoContext *context);
/** @} */
#endif

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* KYMO_RUNTIME_H */
