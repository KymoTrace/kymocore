/* SPDX-License-Identifier: MIT */
/** @file kymo_example_config.h
 * @brief One build-wide configuration for the complete Core example.
 * @par Usage Define KYMO_USER_CONFIG_HEADER for application and library alike.
 */
#ifndef KYMO_EXAMPLE_CONFIG_H
#define KYMO_EXAMPLE_CONFIG_H
#define KYMO_ENABLE_CORE 1
#define KYMO_ENABLE_RUNTIME 1
#define KYMO_ENABLE_PARAMETERS 1
#define KYMO_ENABLE_WATCH 1
#define KYMO_ENABLE_MEMORY 1
#define KYMO_ENABLE_TRACE 1

/** @brief Capacity for the canonical session metadata plus its CRC. */
#define KYMO_MUX_ADMIN_RECORD_MAX 1024u
#define KYMO_ENABLE_DECODER 1
#define KYMO_ENABLE_CRC 1
#endif
