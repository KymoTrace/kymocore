/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */

/**
 * @file kymo_version.h
 * @brief KymoCore library version and configuration contract version.
 * @details Preprocessor-only. Included by kymo_config.h before the optional
 * project header, so that header can test KYMO_CONFIG_API_VERSION.
 * @par Usage
 * Include kymo_modules.h or kymo_config.h; test KYMO_VERSION_MAJOR for API breaks.
 */
#ifndef KYMO_VERSION_H
#define KYMO_VERSION_H

/** @brief Library major version; incremented for incompatible API changes. */
#define KYMO_VERSION_MAJOR 7

/** @brief Library minor version; incremented for compatible additions. */
#define KYMO_VERSION_MINOR 4

/** @brief Library patch version; incremented for compatible fixes. */
#define KYMO_VERSION_PATCH 0

/** @brief Library version as a string literal. */
#define KYMO_VERSION_STRING "7.4.0"

/** @brief Version of the compile-time configuration contract consumed by tools. */
#define KYMO_CONFIG_API_VERSION 2

#endif /* KYMO_VERSION_H */
