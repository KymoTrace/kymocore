/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_types.h
 * @brief Status codes and callback types shared by every KymoCore module.
 *
 * @details Every KymoCore operation that can fail returns KymoStatus, where
 * KYMO_OK (zero) always means success. Telemetry runtime, C++ sender, trace
 * recorder and transport mux all use the same callback types for clock, write,
 * busy and service, so one hardware adapter can serve every module.
 * The header is platform-independent and C/C++ compatible.
 * @par Usage
 * @code{.c}
 * if (Kymo_Init(&context, &config) != KYMO_OK) { handle_configuration_error(); }
 * @endcode
 */
#ifndef KYMO_TYPES_H
#define KYMO_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include "kymo_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @name Status codes
     * @{ */
    /**
     * @brief Result of a KymoCore operation.
     *
     * @details A plain uint8_t keeps the C ABI small and stable. Compare against
     * the KYMO_* status macros; never rely on ordering beyond KYMO_OK being zero.
     * @par Usage
     * KymoStatus status = Kymo_Main(&context);
     */
    typedef uint8_t KymoStatus;

/**
 * @brief Success: the operation completed.
 *
 * @details Init: configuration accepted. Main/Service: a frame was completely accepted.
 * Pop: an event was copied. Queue: the record was queued.
 * @par Usage
 * Continue normally.
 */
#define KYMO_OK 0u

/**
 * @brief Nothing to do right now.
 *
 * @details No channel is due, no trace event is queued, or the mux has nothing to send.
 * @par Usage
 * Treat as normal; call again in a later cycle.
 */
#define KYMO_IDLE 1u

/**
 * @brief Transport or queue is occupied; retry later.
 *
 * @details The driver reported busy, a frame is only partially accepted, or a
 * single-slot queue is still full. No data is lost; pending bytes are resumed.
 * @par Usage
 * Call the same function again in a later cycle.
 */
#define KYMO_BUSY 2u

/**
 * @brief The sample callback reported no measurement.
 *
 * @details Its schedule advances to the next period; no frame is produced.
 * @par Usage
 * Expected when a sensor has no new value.
 */
#define KYMO_SKIPPED 3u

/**
 * @brief Measurement or frame data is invalid.
 *
 * @details Non-finite coordinate, unsupported flags, or a frame that fails
 * decoding. Nothing is written for that data.
 * @par Usage
 * Fix the data source; later calls continue normally.
 */
#define KYMO_BAD_SAMPLE 4u

/**
 * @brief Invalid argument, configuration or uninitialized object.
 *
 * @details Missing required callback, invalid channel table, NULL pointer or
 * an object that was never initialized successfully.
 * @par Usage
 * Correct the configuration and initialize again.
 */
#define KYMO_BAD_CONFIG 5u

/**
 * @brief Transport driver violated its contract; the object is latched faulted.
 *
 * @details A write callback reported more bytes than offered. The stream position
 * is unknown, so no further writes occur until safe reinitialization.
 * @par Usage
 * Repair/reset the driver, ensure no borrowed buffer remains, then reinitialize.
 */
#define KYMO_IO_ERROR 6u

/**
 * @brief Request refused in the current state.
 *
 * @details For example a trace context reset while the context still has open scopes.
 * @par Usage
 * Change the state that blocks the request before retrying.
 */
#define KYMO_REJECTED 7u
    /** @} */

    /** @name Shared callback types
     * @{ */
    /**
     * @brief Read a monotonic millisecond counter.
     *
     * @details The counter may wrap modulo 2^32; KymoCore uses unsigned differences.
     * @param[in,out] user Application context registered with the callback; may be NULL.
     * @return Current uint32 millisecond tick.
     * @par Usage
     * Wrap the platform tick function, for example millis() or HAL_GetTick().
     */
    typedef uint32_t (*KymoClockFn)(void *user);

    /**
     * @brief Accept a prefix of a pending binary frame.
     *
     * @details Zero applies backpressure; short writes resume at the remaining offset.
     * Synchronous drivers copy before returning. Asynchronous drivers may retain the
     * bytes only while the matching KymoBusyFn reports nonzero. Message transports
     * (MQTT, CAN-FD, UDP) accept the whole frame or zero.
     * @param[in,out] user Transport context registered with the callback; may be NULL.
     * @param[in] bytes Non-null readable span owned by KymoCore; must not be modified.
     * @param[in] length Requested byte count, 1..21.
     * @return Accepted byte count in 0..length. Larger counts latch KYMO_IO_ERROR.
     * @par Usage
     * Forward bytes to a UART/USB/TCP driver and report only bytes actually accepted.
     */
    typedef uint8_t (*KymoWriteFn)(void *user, const uint8_t *bytes, uint8_t length);

    /**
     * @brief Report whether the transport still owns the last transmit buffer.
     *
     * @details Optional for copying drivers; mandatory for drivers borrowing the pointer.
     * Also report busy while disconnected. Zero permits reuse of the buffer.
     * @param[in,out] user Transport context registered with the callback.
     * @return Nonzero while unavailable or in flight; zero when the buffer may be reused.
     * @par Usage
     * Read a UART DMA completion flag or USB transmit state.
     */
    typedef uint8_t (*KymoBusyFn)(void *user);

    /**
     * @brief Advance application-side transport maintenance once per cycle.
     *
     * @details Keep execution bounded; must not call back into KymoCore recursively.
     * @param[in,out] user Transport context registered with the callback.
     * @par Usage
     * Poll a network client or driver completion state.
     */
    typedef void (*KymoServiceFn)(void *user);
/** @} */

/**
 * @brief Number of elements of a statically sized array as uint16_t.
 *
 * @details Compile-time only: the operand appears solely inside sizeof and is
 * never evaluated. Pass a true array, not a pointer.
 * @param array_ Array object such as a channel table.
 * @par Usage
 * config.channel_count = KYMO_COUNT_OF(channels);
 */
#define KYMO_COUNT_OF(array_) ((uint16_t)(sizeof(array_) / sizeof((array_)[0])))

#ifdef __cplusplus
}
#endif

#endif /* KYMO_TYPES_H */
