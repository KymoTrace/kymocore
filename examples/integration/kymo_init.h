/* SPDX-License-Identifier: MIT */
/** @file kymo_init.h
 * @brief Common application Init/Main; feature interfaces live separately.
 * @details One statically owned Endpoint and its buffers. Calls are serialized.
 * @par Usage Init, selected feature Init calls, then Main cyclically.
 */
#ifndef EXAMPLE_KYMO_INIT_H
#define EXAMPLE_KYMO_INIT_H
#include <endpoint/kymo_endpoint.h>
/** @brief Construct this project's base configuration.
 * @return Config pointing to persistent buffers and the port callbacks.
 * @par Usage Pass the result to KymoApp_Init. */
KymoEndpointConfig exampleCoreConfig(void);
/** @brief Initialize common resources once.
 * @param[in] config Base configuration; referenced storage must persist.
 * @return Endpoint initialization status. @par Usage Before feature Init. */
KymoStatus KymoApp_Init(const KymoEndpointConfig *config);
/** @brief Run one bounded shared processing cycle.
 * @return Endpoint status; BUSY is normal backpressure.
 * @par Usage The only required cyclic Kymo call. */
KymoStatus KymoApp_Main(void);
/** @brief Stop new work and let subsequent Main calls finish queued output.
 * @return Endpoint stop status. @par Usage Optional at shutdown. */
KymoStatus KymoApp_RequestStop(void);
#endif
