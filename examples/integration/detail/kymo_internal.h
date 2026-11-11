/* SPDX-License-Identifier: MIT */
/** @file kymo_internal.h
 * @brief Private Endpoint access for the feature implementation files.
 * @details Application measurement code should include only its feature header.
 * @par Usage Never initialize or drive the returned Endpoint directly.
 */
#ifndef EXAMPLE_KYMO_INTERNAL_H
#define EXAMPLE_KYMO_INTERNAL_H
#include <endpoint/kymo_endpoint.h>
/** @brief Access the common base. @return Persistent Endpoint storage.
 * @par Usage Pass to the feature's library Init. */
KymoEndpoint *exampleEndpoint(void);
#endif
