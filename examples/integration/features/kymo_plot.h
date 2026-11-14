/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_plot.h
 * @brief Plot configuration and initialization for this project.
 * @details Owns only this feature; transport and scheduling belong to the base.
 * @par Usage Get or fill a config, then call KymoApp_PlotInit once after base Init.
 */
#ifndef EXAMPLE_FEATURE_PLOT_H
#define EXAMPLE_FEATURE_PLOT_H
#include <features/plot/kymo_core_endpoint_plot.h>

/**
 * @brief Construct the demonstration plot configuration.
 * @return Reference to fixed configuration with persistent tables/callbacks.
 * @par Usage Replace demo bindings with application data.
 */
const KymoCorePlotConfig &examplePlotConfig(void);

/**
 * @brief Initialize and register this feature completely.
 * @param[in] config Feature configuration; referenced data must persist.
 * @return Feature initialization status. @par Usage Check before starting Main.
 */
KymoStatus KymoApp_PlotInit(const KymoCorePlotConfig *config);
#endif
