/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_memory.h
 * @brief Memory configuration and initialization for this project.
 * @details Owns only this feature; transport and scheduling belong to the base.
 * @par Usage Get or fill a config, then call KymoApp_MemoryInit once after base Init.
 */
#ifndef EXAMPLE_FEATURE_MEMORY_H
#define EXAMPLE_FEATURE_MEMORY_H
#include <features/memory/kymo_core_endpoint_memory.h>

/**
 * @brief Construct the demonstration memory configuration.
 * @return Reference to fixed configuration with persistent tables/callbacks.
 * @par Usage Replace demo bindings with application data.
 */
const KymoCoreMemoryConfig &exampleMemoryConfig(void);

/**
 * @brief Initialize and register this feature completely.
 * @param[in] config Feature configuration; referenced data must persist.
 * @return Feature initialization status. @par Usage Check before starting Main.
 */
KymoStatus KymoApp_MemoryInit(const KymoCoreMemoryConfig *config);
#endif
