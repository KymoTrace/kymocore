/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_internal.h
 * @brief Private connection access for built-in feature initialization.
 * @details Not included by any public feature header.
 * @par Usage Feature implementation files only.
 */
#ifndef KYMO_CORE_INTERNAL_H
#define KYMO_CORE_INTERNAL_H
#include "../kymo_core_endpoint.h"
#if KYMO_EFFECTIVE_CORE

/**
 * @brief Obtain statically allocated default connection storage.
 * @return Non-NULL internal dispatcher; phase enforces initialization order.
 * @par Usage Pass to existing feature registration functions.
 */
KymoCoreEndpoint *KymoCore_getEndpoint(void);
#endif
#endif
