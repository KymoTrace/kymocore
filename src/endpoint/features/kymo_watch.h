/* SPDX-License-Identifier: MIT */
/** @file kymo_watch.h
 * @brief Watch feature initialization for the shared Endpoint.
 * @details No unrelated feature interfaces. All calls are serialized; tables and
 * callback contexts must outlive the feature. State is caller-owned and must be
 * zero-initialized before Init. The Endpoint owns cyclic processing afterward.
 * @par Usage Initialize the Endpoint, then this feature, then Endpoint Main.
 */
#ifndef KYMO_ENDPOINT_FEATURE_WATCH_H
#define KYMO_ENDPOINT_FEATURE_WATCH_H
#include "../detail/kymo_feature.h"
#include "../../control/kymo_watch.h"
#if (KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_WATCH) || defined(DOXYGEN)
#ifdef __cplusplus
extern "C" {
#endif
/** @brief Watch-only settings; no physical writer or other feature configs.
 * @details Copied by Init; referenced arrays and contexts remain caller-owned.
 * @par Usage Zero-initialize and assign named fields. */
typedef struct {
    KymoWatchConfig watch; /**< Readable ranges, build ID and reader. */
    uint8_t first_channel; /**< First allowed channel, inclusive. */
    uint8_t last_channel; /**< Last allowed channel, inclusive; entire range is reserved. */
} KymoEndpointWatchConfig;
/** @brief Persistent Watch context; exposed solely for static allocation.
 * @details Internal fields are read-only; never move/reinitialize a bound feature.
 * @par Usage Declare static storage or zero-initialize before the first Init. */
typedef struct {
    KymoEndpointFeature binding; /**< Internal ownership and dispatch. */
    KymoWatch watch; /**< Owned live-variable receiver. */
} KymoEndpointWatch;
/** @brief Initialize and register Watch completely, without physical I/O.
 * @details One instance per feature type per Endpoint. Failed setup latches a
 * configuration error on a configuring Endpoint; late setup leaves a running
 * Endpoint unchanged. Shared channel reservations reject cross-feature overlap.
 * @param[in,out] endpoint Base in configuring phase.
 * @param[in,out] feature Zero-initialized feature storage.
 * @param[in] config Watch settings; NULL is invalid.
 * @return OK, BAD_CONFIG for invalid resources, REJECTED for duplicate/late
 * initialization, or the existing terminal error.
 * @par Usage Check success before starting the common Main loop. */
KymoStatus KymoEndpointWatch_Init(KymoEndpoint *endpoint, KymoEndpointWatch *feature,
    const KymoEndpointWatchConfig *config);
#ifdef __cplusplus
}
#endif
#endif
#endif /* KYMO_ENDPOINT_FEATURE_WATCH_H */
