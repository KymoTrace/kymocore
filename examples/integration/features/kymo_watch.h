/* SPDX-License-Identifier: MIT */
/** @file kymo_watch.h
 * @brief Watch configuration and initialization for this project.
 * @details Owns only this feature; transport and scheduling belong to the base.
 * @par Usage Get or fill a config, then call KymoApp_WatchInit once after base Init.
 */
#ifndef EXAMPLE_FEATURE_WATCH_H
#define EXAMPLE_FEATURE_WATCH_H
#include <endpoint/features/kymo_watch.h>
/** @brief Construct the demonstration watch configuration.
 * @return Config referring to persistent tables/callbacks.
 * @par Usage Replace demo bindings with application data. */
KymoEndpointWatchConfig exampleWatchConfig(void);
/** @brief Initialize and register this feature completely.
 * @param[in] config Feature configuration; referenced data must persist.
 * @return Feature initialization status. @par Usage Check before starting Main. */
KymoStatus KymoApp_WatchInit(const KymoEndpointWatchConfig *config);
#endif
