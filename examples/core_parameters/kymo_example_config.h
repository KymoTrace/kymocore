/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_example_config.h
 * @brief Shared settings for both the application and all library sources.
 * @par Usage Select through KYMO_USER_CONFIG_HEADER in the build, not main alone.
 */
#ifndef KYMO_EXAMPLE_CONFIG_H
#define KYMO_EXAMPLE_CONFIG_H
#define KYMO_ENABLE_CORE 1
#define KYMO_ENABLE_RUNTIME 0
#define KYMO_ENABLE_PARAMETERS 1
#define KYMO_ENABLE_DECODER 1
#define KYMO_ENABLE_CRC 1
#endif
