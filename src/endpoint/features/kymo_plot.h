/* SPDX-License-Identifier: MIT */
/** @file kymo_plot.h
 * @brief Plot feature initialization for the shared Endpoint.
 * @details No unrelated feature interfaces. All calls are serialized; tables and
 * callback contexts must outlive the feature. State is caller-owned and must be
 * zero-initialized before Init. The Endpoint owns cyclic processing afterward.
 * @par Usage Initialize the Endpoint, then this feature, then Endpoint Main.
 */
#ifndef KYMO_ENDPOINT_FEATURE_PLOT_H
#define KYMO_ENDPOINT_FEATURE_PLOT_H
#include "../detail/kymo_feature.h"
#include "../../telemetry/kymo_runtime.h"
#if (KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_RUNTIME) || defined(DOXYGEN)
#ifdef __cplusplus
extern "C"
{
#endif
    /** @brief Plot-only settings; no physical writer or other feature configs.
     * @details Copied by Init; referenced arrays and contexts remain caller-owned.
     * @par Usage Zero-initialize and assign named fields. */
    typedef struct
    {
        const KymoChannel *channels; /**< Persistent channel table. */
        uint16_t channel_count;      /**< Number of channels. */
        KymoSampleFn sample;         /**< Required sample reader. */
        void *sample_user;           /**< Sample reader context. */
    } KymoEndpointPlotConfig;
    /** @brief Persistent Plot context; exposed solely for static allocation.
     * @details Internal fields are read-only; never move/reinitialize a bound feature.
     * @par Usage Declare static storage or zero-initialize before the first Init. */
    typedef struct
    {
        KymoEndpointFeature binding; /**< Internal ownership and dispatch. */
        KymoContext runtime;         /**< Owned telemetry runtime. */
    } KymoEndpointPlot;
    /** @brief Initialize and register Plot completely, without physical I/O.
     * @details One instance per feature type per Endpoint. Failed setup latches a
     * configuration error on a configuring Endpoint; late setup leaves a running
     * Endpoint unchanged. Shared channel reservations reject cross-feature overlap.
     * Timestamped channels use the Endpoint device clock directly, matching Param,
     * Watch and Memory; they are not rebased to this feature's initialization time.
     * @param[in,out] endpoint Base in configuring phase.
     * @param[in,out] feature Zero-initialized feature storage.
     * @param[in] config Plot settings; NULL is invalid.
     * @return OK, BAD_CONFIG for invalid resources, REJECTED for duplicate/late
     * initialization, or the existing terminal error.
     * @par Usage Check success before starting the common Main loop. */
    KymoStatus KymoEndpointPlot_Init(KymoEndpoint *endpoint, KymoEndpointPlot *feature,
                                     const KymoEndpointPlotConfig *config);
#ifdef __cplusplus
}
#endif
#endif
#endif /* KYMO_ENDPOINT_FEATURE_PLOT_H */
