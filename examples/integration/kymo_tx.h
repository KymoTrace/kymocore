/* SPDX-License-Identifier: MIT */
/** @file kymo_tx.h
 * @brief Application-local single-frame sender; no module scheduling or Endpoint.
 * @details Static storage, single caller, no reset while a driver owns bytes.
 * @par Usage Submit one complete frame, then service once per cycle.
 */
#ifndef EXAMPLE_KYMO_TX_H
#define EXAMPLE_KYMO_TX_H
#include <stddef.h>
#include <stdint.h>
#include <kymo_types.h>

/** @brief Capacity of the application-owned frame copy in bytes.
 * @par Usage Reject oversized frames before submission.
 */
#define EXAMPLE_TX_CAPACITY 240u

/** @brief Copy a complete frame into free persistent storage.
 * @param[in] bytes Readable frame, copied before return; NULL is invalid.
 * @param[in] length Frame size, 1..EXAMPLE_TX_CAPACITY.
 * @return KYMO_OK if copied, KYMO_BUSY without consuming anything, KYMO_BAD_CONFIG
 * for invalid input, or latched KYMO_IO_ERROR after a driver violation.
 * @par Usage After KYMO_OK the caller may immediately reuse its own buffer.
 */
KymoStatus exampleTxSubmit(const uint8_t *bytes, size_t length);

/** @brief Advance by at most one physical write attempt.
 * @details Retains storage until every byte is accepted AND the port releases it.
 * A driver accepting more than offered permanently stops this sender. There is
 * deliberately no timeout/reset that could invalidate a borrowed buffer.
 * @return KYMO_IDLE when storage is free, KYMO_BUSY while queued/borrowed, or
 * KYMO_IO_ERROR after a driver contract violation.
 * @par Usage Produce another frame only after KYMO_IDLE.
 */
KymoStatus exampleTxService(void);
#endif
