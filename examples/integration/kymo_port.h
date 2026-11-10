/* SPDX-License-Identifier: MIT */
/** @file kymo_port.h
 * @brief Application-owned port; no library types or hardware SDKs.
 * @details Single-threaded. The supplied native adapter reads/writes binary files
 * and simulates time and delayed pointer ownership. File I/O is a host harness,
 * not a real-time driver. On hardware replace it with bounded UART/USB operations.
 * @par Usage Initialize, service once per cycle, read/write only while healthy.
 */
#ifndef EXAMPLE_KYMO_PORT_H
#define EXAMPLE_KYMO_PORT_H
#include <stddef.h>
#include <stdint.h>

/** @brief Advance the platform transport once; native port also advances time by 1 ms.
 * @par Usage Once per main-loop iteration; never recursively from write.
 */
void examplePortService(void);
/** @brief Read a bounded prefix of requests; zero at EOF or on error.
 * @param[out] output Destination; NULL returns zero.
 * @param[in] capacity Writable byte count.
 * @return Copied bytes, at most capacity (native adapter fragments to 3 bytes).
 * @par Usage Feed the same bytes to both existing control receivers.
 */
size_t examplePortRead(uint8_t *output, size_t capacity);
/** @brief Borrow a prefix until busy clears; zero means retry later.
 * @param[in] bytes Stable storage, retained while busy; NULL returns zero.
 * @param[in] length Available bytes.
 * @return Accepted prefix, 0..length; larger is a fatal driver contract error.
 * @par Usage Call only when busy is zero; preserve the whole frame meanwhile.
 */
size_t examplePortWrite(const uint8_t *bytes, size_t length);
/** @brief Test buffer ownership or temporary unavailability.
 * @return 1 while borrowed/unavailable; 0 permits a write and buffer reuse.
 * @par Usage Check before writing or changing the send buffer.
 */
uint8_t examplePortBusy(void);
/** @brief Millisecond callback, wrapping modulo 2^32.
 * @param[in] user Unused, may be NULL.
 * @return Current simulated milliseconds.
 * @par Usage Bind to KymoConfig::clock_ms.
 */
uint32_t examplePortClockMs(void *user);
/** @brief Trace clock at 1000 ticks/second.
 * @param[in] user Unused, may be NULL.
 * @return Monotonic 64-bit ticks.
 * @par Usage Bind to KymoTraceConfig::clock_ticks; keep metadata consistent.
 */
uint64_t examplePortClockTicks(void *user);
/** @brief Report native file/ownership errors.
 * @return 1 while healthy, 0 after an error.
 * @par Usage Stop production on failure; do not reset borrowed storage.
 */
uint8_t examplePortHealthy(void);
#endif
