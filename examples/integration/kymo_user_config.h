/* SPDX-License-Identifier: MIT */
/** @file kymo_user_config.h
 * @brief Build settings shared by ALL application and library translation units.
 * @details This reference intentionally enables all five integrated modules.
 * @par Usage Pass KYMO_USER_CONFIG_HEADER=\"kymo_user_config.h\" to the whole build.
 */
#ifndef EXAMPLE_KYMO_USER_CONFIG_H
#define EXAMPLE_KYMO_USER_CONFIG_H

/** @brief Capacity of this application's shared frame storage, in bytes.
 * @par Usage Size both the application buffer and the native borrow snapshot.
 */
#define EXAMPLE_TX_CAPACITY 240u

/** @brief Enable the shared feature coordinator. */
#ifndef KYMO_ENABLE_ENDPOINT
#define KYMO_ENABLE_ENDPOINT 1
#endif
/** @brief Enable KymoCore's shared frame sender. */
#ifndef KYMO_ENABLE_TX
#define KYMO_ENABLE_TX 1
#endif
/** @brief Enable CRC-protected telemetry. */
#ifndef KYMO_ENABLE_CRC
#define KYMO_ENABLE_CRC 1
#endif
/** @brief Required by parameter and watch reception. */
#ifndef KYMO_ENABLE_DECODER
#define KYMO_ENABLE_DECODER 1
#endif
/** @brief Enable writable parameters. */
#ifndef KYMO_ENABLE_PARAMETERS
#define KYMO_ENABLE_PARAMETERS 1
#endif
/** @brief Enable live variables. */
#ifndef KYMO_ENABLE_WATCH
#define KYMO_ENABLE_WATCH 1
#endif
/** @brief Enable memory telemetry. */
#ifndef KYMO_ENABLE_MEMORY
#define KYMO_ENABLE_MEMORY 1
#endif
/** @brief Enable single-context execution tracing, without RTOS/ISR hooks. */
#ifndef KYMO_ENABLE_TRACE
#define KYMO_ENABLE_TRACE 1
#endif
/** @brief One scheduled measurement channel. */
#ifndef KYMO_MAX_CHANNELS
#define KYMO_MAX_CHANNELS 1u
#endif
/** @brief One writable gain. */
#ifndef KYMO_MAX_PARAMETERS
#define KYMO_MAX_PARAMETERS 1u
#endif
/** @brief One live variable slot. */
#ifndef KYMO_MAX_WATCHES
#define KYMO_MAX_WATCHES 1u
#endif
/** @brief One synthetic memory metric. */
#ifndef KYMO_MAX_MEMORY_ENTRIES
#define KYMO_MAX_MEMORY_ENTRIES 1u
#endif
/** @brief Bounded trace storage. */
#ifndef KYMO_TRACE_EVENTS_PER_CORE
#define KYMO_TRACE_EVENTS_PER_CORE 16u
#endif
/** @brief The example has one main-loop context. */
#ifndef KYMO_TRACE_MAX_CONTEXTS
#define KYMO_TRACE_MAX_CONTEXTS 1u
#endif
/** @brief No deeply nested instrumentation in this example. */
#ifndef KYMO_TRACE_MAX_DEPTH
#define KYMO_TRACE_MAX_DEPTH 4u
#endif
/** @brief Fits the example session JSON plus its CRC32. */
#ifndef KYMO_MUX_ADMIN_RECORD_MAX
#define KYMO_MUX_ADMIN_RECORD_MAX 1024u
#endif
#endif
