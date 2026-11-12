/* SPDX-License-Identifier: MIT */
/** @file kymo_param.h
 * @brief Param feature initialization for the shared Endpoint.
 * @details No unrelated feature interfaces. All calls are serialized; tables and
 * callback contexts must outlive the feature. State is caller-owned and must be
 * zero-initialized before Init. The Endpoint owns cyclic processing afterward.
 * @par Usage Initialize the Endpoint, then this feature, then Endpoint Main.
 */
#ifndef KYMO_ENDPOINT_FEATURE_PARAM_H
#define KYMO_ENDPOINT_FEATURE_PARAM_H
#include "../detail/kymo_feature.h"
#include "../../control/kymo_param.h"
#if (KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_PARAMETERS) || defined(DOXYGEN)
#ifdef __cplusplus
extern "C"
{
#endif
    /** @brief Param-only settings; no physical writer or other feature configs.
     * @details Copied by Init; referenced arrays and contexts remain caller-owned.
     * @par Usage Zero-initialize and assign named fields. */
    typedef struct
    {
        const KymoParameter *table; /**< Persistent parameter table. */
        size_t count;               /**< Number of parameters. */
    } KymoEndpointParamConfig;
    /** @brief Persistent Param context; exposed solely for static allocation.
     * @details Internal fields are read-only; never move/reinitialize a bound feature.
     * @par Usage Declare static storage or zero-initialize before the first Init. */
    typedef struct
    {
        KymoEndpointFeature binding; /**< Internal ownership and dispatch. */
        KymoParamReceiver receiver;  /**< Owned parameter receiver. */
    } KymoEndpointParam;
    /** @brief Initialize and register Param completely, without physical I/O.
     * @details One instance per feature type per Endpoint. Failed setup latches a
     * configuration error on a configuring Endpoint; late setup leaves a running
     * Endpoint unchanged. Shared channel reservations reject cross-feature overlap.
     * @param[in,out] endpoint Base in configuring phase.
     * @param[in,out] feature Zero-initialized feature storage.
     * @param[in] config Param settings; NULL is invalid.
     * @return OK, BAD_CONFIG for invalid resources, REJECTED for duplicate/late
     * initialization, or the existing terminal error.
     * @par Usage Check success before starting the common Main loop. */
    KymoStatus KymoEndpointParam_Init(KymoEndpoint *endpoint, KymoEndpointParam *feature,
                                      const KymoEndpointParamConfig *config);
#ifdef __cplusplus
}
#endif
#endif
#endif /* KYMO_ENDPOINT_FEATURE_PARAM_H */
