/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_endpoint_memory.h
 * @brief Memory feature initialization for the shared Endpoint.
 * @details No unrelated feature interfaces. All calls are serialized; tables and
 * callback contexts must outlive the feature. State is caller-owned and must be
 * zero-initialized before Init. The Endpoint owns cyclic processing afterward.
 * @par Usage Initialize the Endpoint, then this feature, then Endpoint Main.
 */
#ifndef KYMO_CORE_ENDPOINT_MEMORY_H
#define KYMO_CORE_ENDPOINT_MEMORY_H
#include "../../core/detail/kymo_core_endpoint_feature.h"
#include "kymo_memory.h"
#if (KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_MEMORY) || defined(DOXYGEN)
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Persistent Memory context; exposed solely for static allocation.
     * @details Internal fields are read-only; never move/reinitialize a bound feature.
     * @par Usage Declare static storage or zero-initialize before the first Init. */
    typedef struct
    {
        /** @brief Internal ownership and dispatch. */
        KymoCoreEndpointFeature binding;

        /** @brief Owned memory sampler. */
        KymoMemory memory;
    } KymoCoreEndpointMemory;

    /**
     * @brief Initialize and register Memory completely, without physical I/O.
     * @details One instance per feature type per Endpoint. Failed setup latches a
     * configuration error on a configuring Endpoint; late setup leaves a running
     * Endpoint unchanged. Shared channel reservations reject cross-feature overlap.
     * @param[in,out] endpoint Base in configuring phase.
     * @param[in,out] feature Zero-initialized feature storage.
     * @param[in] config Memory settings; NULL is invalid.
     * @return OK, BAD_CONFIG for invalid resources, REJECTED for duplicate/late
     * initialization, or the existing terminal error.
     * @par Usage Check success before starting the common Main loop.
     */
    KymoStatus KymoCoreEndpointMemory_init(KymoCoreEndpoint *endpoint,
                                           KymoCoreEndpointMemory *feature,
                                           const KymoCoreMemoryConfig *config);
#ifdef __cplusplus
}
#endif
#endif
#endif /* KYMO_CORE_ENDPOINT_MEMORY_H */
