/* SPDX-License-Identifier: MIT */
/** @file kymo_bindings.h
 * @brief Application measurements, parameters and module configuration.
 * @details All referenced tables live for the program's lifetime. The example
 * reserves channel 0 for measurements, 100 for gain, 150 for watch, 220 for memory.
 * @par Usage Copy the configs during startup, then attach the application writer.
 */
#ifndef EXAMPLE_KYMO_BINDINGS_H
#define EXAMPLE_KYMO_BINDINGS_H
#include <KymoCore.h>

/** @brief Number of entries in the persistent parameter table.
 * @par Usage Array bound; pass KYMO_COUNT_OF(exampleParameters) when initializing.
 */
static const size_t EXAMPLE_PARAMETER_COUNT = 1u;
/** @brief Stable session identity for this finite replay, not a real boot ID.
 * @par Usage Used by the recorder; the independent wire test expects this ID.
 */
static const uint64_t EXAMPLE_TRACE_SESSION_ID = 1u;
/** @brief Clock domain described by the canonical session metadata.
 * @par Usage Keep synchronized with kymo_trace_records.h and its wire test.
 */
static const uint16_t EXAMPLE_TRACE_CLOCK_DOMAIN_ID = 0u;
/** @brief Superloop context declared in the trace dictionary.
 * @par Usage Pass to KymoTrace_Mark; update the dictionary when changing it.
 */
static const uint32_t EXAMPLE_TRACE_CONTEXT_ID = 1u;
/** @brief Main-cycle marker declared in the trace dictionary.
 * @par Usage Pass to KymoTrace_Mark; update the dictionary when changing it.
 */
static const uint16_t EXAMPLE_TRACE_MARKER_ID = 1u;

/** @brief Persistent writable gain table, ID 100, range 0..10.
 * @par Usage KymoParam_Init(&receiver, exampleParameters, KYMO_COUNT_OF(exampleParameters)).
 */
extern const KymoParameter exampleParameters[EXAMPLE_PARAMETER_COUNT];
/** @brief Create runtime bindings; write must be supplied by main.
 * @return Zero-initialized configuration with channel, clock and sample bindings.
 * @par Usage Set config.write, then Kymo_Init.
 */
KymoConfig exampleTelemetryBindings(void);
/** @brief Create a safe virtual-address mapping for the live gain.
 * @return Watch configuration; no native pointer is truncated to a wire address.
 * @par Usage Pass to KymoWatch_Init.
 */
KymoWatchConfig exampleWatchBindings(void);
/** @brief Create a synthetic free-memory metric (not an actual heap measurement).
 * @return Memory configuration for channel 220.
 * @par Usage Pass to KymoMemory_Init; replace its reader on a device.
 */
KymoMemoryConfig exampleMemoryBindings(void);
/** @brief Create single-threaded trace clock/session bindings.
 * @return Trace configuration; session 1 is deliberately fixed for this replay.
 * @par Usage Pass to KymoTrace_Init; real boots need a new session identity.
 */
KymoTraceConfig exampleTraceBindings(void);
#endif
