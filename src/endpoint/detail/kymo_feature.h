/* SPDX-License-Identifier: MIT */
/** @file kymo_feature.h
 * @brief Storage declarations shared by Endpoint feature contexts.
 * @details Internal fields are exposed for static allocation, not registration.
 * @par Usage Include a public feature header; never fill this structure manually.
 */
#ifndef KYMO_ENDPOINT_DETAIL_FEATURE_H
#define KYMO_ENDPOINT_DETAIL_FEATURE_H
#include "../../kymo_types.h"
#if KYMO_EFFECTIVE_ENDPOINT || defined(DOXYGEN)
#ifdef __cplusplus
extern "C" {
#endif
/** @brief Forward declaration of the shared coordinator.
 * @par Usage Pass its address to a feature Init. */
typedef struct KymoEndpoint KymoEndpoint;
/** @brief Internal feature binding; zero-initialize with its owning feature.
 * @details All fields belong to the library after a successful feature Init.
 * @par Usage Do not modify, copy or move a bound feature. */
typedef struct KymoEndpointFeature {
    /** @brief Coordinator owning this binding. */
    KymoEndpoint *endpoint;
    /** @brief Owning feature context. */
    void *user;
    /** @brief Produce one source turn; called only with free TX storage. */
    KymoStatus (*produce)(void *user, uint8_t source, uint32_t now, uint8_t draining);
    /** @brief Optional incoming-byte consumer. */
    void (*feed)(void *user, const uint8_t *bytes, size_t length);
    /** @brief Optional producer-stop notification; performs no physical I/O. */
    void (*stop)(void *user);
    /** @brief Stable diagnostic feature identifier. */
    uint8_t id;
    /** @brief Number of source turns belonging to this feature. */
    uint8_t sources;
} KymoEndpointFeature;
#ifdef __cplusplus
}
#endif
#endif
#endif
