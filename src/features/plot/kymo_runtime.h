/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_runtime.h
 * @brief Cyclic telemetry runtime: describe channels once, call KymoRuntime_main forever.
 *
 * @details The application answers four questions and KymoCore does the rest:
 * - WHAT:  a channel table (ID, period, optional X/Z/timestamp fields),
 * - WHEN:  a millisecond clock callback,
 * - VALUE: a sample callback that fills one KymoSample per channel ID,
 * - WHERE: a write callback that hands frame bytes to any transport.
 *
 * KymoRuntime_init validates and copies the configuration into the context; only the
 * channel table must stay alive. KymoRuntime_main performs one bounded step: it
 * resumes a partial write or samples at most one due channel. No heap, no
 * blocking, no SDK types. Available with KYMO_ENABLE_RUNTIME=1 (default).
 * @defgroup kymo_runtime Telemetry runtime
 * @{
 * @par Usage
 * @code{.c}
 * static const KymoChannel channels[] = {{20u, 0u, 0u}};  // periodMs, id, flags
 * static KymoContext context;
 * void setup(void) {
 *     KymoConfig config = {0};
 *     config.channels = channels;
 *     config.channel_count = KYMO_COUNT_OF(channels);
 *     config.clock_ms = read_clock_ms;
 *     config.sample = read_sample;
 *     config.write = write_bytes;
 *     (void)KymoRuntime_init(&context, &config);
 * }
 * void loop(void) { (void)KymoRuntime_main(&context); }
 * @endcode
 */
#ifndef KYMO_RUNTIME_H
#define KYMO_RUNTIME_H

#include "detail/kymo_plot_types.h"
#include "../../communication/kymo_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @name Channel and sample data
     * @{ */

    /** @} */

    /**
     * @name Configuration and runtime storage
     * @{ */
    /**
     * @brief Runtime wiring: required fields first, optional fields may stay zero.
     *
     * @details Input to KymoRuntime_init, which validates and copies it into the context.
     * The configuration object itself may therefore be a local variable; the channel
     * table and every user context must stay alive while the runtime is used.
     * Fill the fields by name starting from a zero-initialized object.
     * @par Usage
     * KymoConfig config = {0}; config.channels = channels; ... KymoRuntime_init(&context, &config);
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
        KymoClockMsFn clock_ms;

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
        KymoIsBusyFn busy;

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
     * static KymoContext context; then KymoRuntime_init(&context, &config).
     */
    typedef struct
    {
        /** @brief Validated copy of the configuration passed to KymoRuntime_init. */
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

        /** @brief Nonzero after a successful KymoRuntime_init. */
        uint8_t initialized;
    } KymoContext;

    /** @} */

#if KYMO_EFFECTIVE_RUNTIME || defined(DOXYGEN)

    /**
     * @name Lifecycle
     * @{
     *
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
     * Initialize hardware, then check KymoRuntime_init(&context, &config) == KYMO_OK.
     */
    KymoStatus KymoRuntime_init(KymoContext *context, const KymoConfig *config);

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
    KymoStatus KymoRuntime_main(KymoContext *context);

    /**
     * @brief Ready-made KymoSampleFn that reads a bound float variable.
     *
     * @details Looks up id in the binding's channel table (at most count
     * comparisons) and copies the variable into sample->value. Accepts scalar
     * channels with an optional timestamp; X/Z channels are skipped. The value is read
     * when the channel is due; update variables from the context that calls
     * KymoRuntime_main. Uses no runtime state, so it also serves KymoCorePlotConfig.
     * @param[in] user A KymoPlotBinding; NULL yields zero.
     * @param[in] id Channel ID to look up.
     * @param[out] sample Destination; NULL yields zero.
     * @return One if the ID is bound to a non-null variable with scalar flags,
     * otherwise zero (the runtime reports KYMO_SKIPPED).
     * @par Usage
     * config.sample = KymoRuntime_sampleBound; config.sample_user = &binding;
     */
    uint8_t KymoRuntime_sampleBound(void *user, uint8_t id, KymoSample *sample);

/** @} */
#endif

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* KYMO_RUNTIME_H */
