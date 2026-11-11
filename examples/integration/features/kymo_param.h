/* SPDX-License-Identifier: MIT */
/** @file kymo_param.h
 * @brief Param configuration and initialization for this project.
 * @details Owns only this feature; transport and scheduling belong to the base.
 * @par Usage Get or fill a config, then call KymoApp_ParamInit once after base Init.
 */
#ifndef EXAMPLE_FEATURE_PARAM_H
#define EXAMPLE_FEATURE_PARAM_H
#include <endpoint/features/kymo_param.h>
/** @brief Construct the demonstration param configuration.
 * @return Config referring to persistent tables/callbacks.
 * @par Usage Replace demo bindings with application data. */
KymoEndpointParamConfig exampleParamConfig(void);
/** @brief Initialize and register this feature completely.
 * @param[in] config Feature configuration; referenced data must persist.
 * @return Feature initialization status. @par Usage Check before starting Main. */
KymoStatus KymoApp_ParamInit(const KymoEndpointParamConfig *config);
#endif
