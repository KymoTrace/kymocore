/* SPDX-License-Identifier: MIT */
/** @file kymo_app.h
 * @brief Lifecycle of this fixed example application, independent of its host.
 * @details Wires the five concrete modules; not a configurable library facade.
 * All calls are serialized in one task. Port startup/service belongs to the host.
 * @par Usage Init once, Cycle repeatedly, then Drain until idle at shutdown.
 */
#ifndef EXAMPLE_KYMO_APP_H
#define EXAMPLE_KYMO_APP_H
#include <kymo_types.h>

/** @brief Initialize the fixed measurement/control/trace bindings once.
 * @return KYMO_OK on success, an initialization error otherwise; KYMO_REJECTED
 * on a repeated call (never resets state or borrowed buffers).
 * @par Usage After platform/port startup, before the first Cycle.
 */
KymoStatus exampleAppInit(void);
/** @brief Receive a bounded chunk and give one source a send turn if possible.
 * @return KYMO_OK for a normal cycle (including backpressure), a latched module
 * error on failure, KYMO_BAD_CONFIG before Init, KYMO_REJECTED after Drain begins.
 * @par Usage Call after the platform's port service from one cyclic task.
 */
KymoStatus exampleAppCycle(void);
/** @brief Stop production and incrementally finish buffered output and trace.
 * @details No new requests/samples are consumed; only already queued output and
 * recorded trace are drained. No clock advancement, deadline or busy waiting.
 * @return KYMO_BUSY while draining, KYMO_IDLE after the settled session end and
 * physical buffer release, an error on failure, KYMO_BAD_CONFIG before Init.
 * @par Usage At shutdown, service the port and call again until KYMO_IDLE.
 */
KymoStatus exampleAppDrain(void);
#endif
