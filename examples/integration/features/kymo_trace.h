/* SPDX-License-Identifier: MIT */
/** @file kymo_trace.h
 * @brief Trace configuration and initialization for this project.
 * @details Owns only this feature; transport and scheduling belong to the base.
 * @par Usage Get or fill a config, then call KymoApp_TraceInit once after base Init.
 */
#ifndef EXAMPLE_FEATURE_TRACE_H
#define EXAMPLE_FEATURE_TRACE_H
#include <endpoint/features/kymo_trace.h>
/** @brief Construct the demonstration trace configuration.
 * @return Config referring to persistent tables/callbacks.
 * @par Usage Replace demo bindings with application data. */
KymoEndpointTraceConfig exampleTraceConfig(void);
/** @brief Initialize and register this feature completely.
 * @param[in] config Feature configuration; referenced data must persist.
 * @return Feature initialization status. @par Usage Check before starting Main. */
KymoStatus KymoApp_TraceInit(const KymoEndpointTraceConfig *config);
/** @brief Record the demo marker when session descriptions are ready.
 * @param[in] cycle Application cycle counter.
 * @par Usage At the measured application location, independently of Main. */
void exampleTraceCycle(uint32_t cycle);
#endif
