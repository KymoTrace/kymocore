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

/** @brief Persistent writable gain table, ID 100, range 0..10.
 * @par Usage KymoParam_Init(&receiver, exampleParameters, 1u).
 */
extern const KymoParameter exampleParameters[1];
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
