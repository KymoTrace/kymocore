/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_plot.h
 * @brief Complete application interface for the default Core plot feature.
 * @details Configuration is copied; referenced tables, buffers and callback contexts
 * must remain alive. No heap or application-owned dispatcher state is required.
 * @par Usage Enable the feature, initialize Core, initialize this feature, call Core Main.
 */
#ifndef KYMO_CORE_PLOT_H
#define KYMO_CORE_PLOT_H
#include "kymo_types.h"
#if KYMO_EFFECTIVE_RUNTIME || defined(DOXYGEN)
#include "features/plot/detail/kymo_plot_types.h"
#include "communication/detail/kymo_data_flags.h"
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Application configuration for plot.
     * @details Callback fields precede contexts and data parameters.
     * @par Usage Zero-initialize, assign named fields, and keep referenced storage alive.
     */
    typedef struct
    {
        /** @brief Required sample callback; writes one finite sample or returns zero to skip. */
        KymoSampleFn sampleCbk;

        /** @brief Opaque callback context, allowed to be NULL. */
        void *pSampleContext;

        /** @brief Persistent, uniquely identified channels. */
        const KymoChannel *pChannelsAv;

        /** @brief Number of channels, 1..KYMO_MAX_CHANNELS. */
        uint16_t channelCount;
    } KymoCorePlotConfig;
#if KYMO_EFFECTIVE_CORE || defined(DOXYGEN)

    /**
     * @brief Initialize plot and activate its processing in Core Main.
     * @details Call once after Core Init and before the first Main. Setup errors latch
     * a Core fault; late calls are rejected without altering the running Core.
     * No callbacks are invoked periodically until Core Main runs. Not ISR-safe.
     * @param[in] pConfig Persistent referenced data and callbacks; NULL is invalid.
     * @return OK, BAD_CONFIG, REJECTED, or an existing Core error.
     * @par Usage Check success before entering the common main loop.
     */
    KymoStatus KymoCorePlot_init(const KymoCorePlotConfig *pConfig);

    /**
     * @brief Optional sample callback for a table of float pointers.
     * @param[in] pContext Persistent KymoPlotBinding, or NULL to skip.
     * @param[in] channelId Requested configured channel.
     * @param[out] pSample Destination, or NULL to skip.
     * @return Nonzero if a scalar value was obtained, zero otherwise.
     * @par Usage Set sampleCbk to this function and pSampleContext to a binding.
     */
    uint8_t KymoCorePlot_sampleBound(void *pContext, uint8_t channelId, KymoSample *pSample);
#endif
#ifdef __cplusplus
}
#endif
#endif
#endif
