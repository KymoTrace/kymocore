/* SPDX-License-Identifier: MIT */
/** @file kymo_memory.h
 * @brief Memory feature initialization for the shared Endpoint.
 * @details No unrelated feature interfaces. All calls are serialized; tables and
 * callback contexts must outlive the feature. State is caller-owned and must be
 * zero-initialized before Init. The Endpoint owns cyclic processing afterward.
 * @par Usage Initialize the Endpoint, then this feature, then Endpoint Main.
 */
#ifndef KYMO_ENDPOINT_FEATURE_MEMORY_H
#define KYMO_ENDPOINT_FEATURE_MEMORY_H
#include "../detail/kymo_feature.h"
#include "../../telemetry/kymo_memory.h"
#if (KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_MEMORY) || defined(DOXYGEN)
#ifdef __cplusplus
extern "C" {
#endif
/** @brief Memory settings, using the existing module configuration.
 * @par Usage Assign entries, reader and periods; the Endpoint supplies time. */
typedef KymoMemoryConfig KymoEndpointMemoryConfig;
/** @brief Persistent Memory context; exposed solely for static allocation.
 * @details Internal fields are read-only; never move/reinitialize a bound feature.
 * @par Usage Declare static storage or zero-initialize before the first Init. */
typedef struct {
    KymoEndpointFeature binding; /**< Internal ownership and dispatch. */
    KymoMemory memory; /**< Owned memory sampler. */
} KymoEndpointMemory;
/** @brief Initialize and register Memory completely, without physical I/O.
 * @details One instance per feature type per Endpoint. Failed setup latches a
 * configuration error on a configuring Endpoint; late setup leaves a running
 * Endpoint unchanged. Shared channel reservations reject cross-feature overlap.
 * @param[in,out] endpoint Base in configuring phase.
 * @param[in,out] feature Zero-initialized feature storage.
 * @param[in] config Memory settings; NULL is invalid.
 * @return OK, BAD_CONFIG for invalid resources, REJECTED for duplicate/late
 * initialization, or the existing terminal error.
 * @par Usage Check success before starting the common Main loop. */
KymoStatus KymoEndpointMemory_Init(KymoEndpoint *endpoint, KymoEndpointMemory *feature,
    const KymoEndpointMemoryConfig *config);
#ifdef __cplusplus
}
#endif
#endif
#endif /* KYMO_ENDPOINT_FEATURE_MEMORY_H */
