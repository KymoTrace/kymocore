/* SPDX-License-Identifier: MIT */
/** @file kymo_trace.cpp
 * @brief Trace startup, bounded output and finite shutdown behind one feature.
 */
#include "kymo_trace.h"
#include "../detail/kymo_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE
enum TraceMetadataStage
{
    SESSION_METADATA,
    SYMBOL_DICTIONARY,
    METADATA_COMPLETE
};

/*******************************************************************************
 * stopTrace
 ******************************************************************************/
static void stopTrace(void *user)
{
    KymoEndpointTrace *feature = static_cast<KymoEndpointTrace *>(user);
    feature->mux.config.health_period_ticks = 0u;
}

/*******************************************************************************
 * produceMetadata
 ******************************************************************************/
static KymoStatus produceMetadata(KymoEndpointTrace *feature, uint8_t draining)
{
    KymoStatus status = KYMO_OK;
    if ((feature->mux.admin_pending == 0u) && (feature->mux.pending == 0u))
    {
        uint8_t session = (uint8_t)(feature->metadata_stage == SESSION_METADATA);
        status = KymoMux_QueueAdmin(
            &feature->mux,
            session != 0u ? KYMO_TRACE_PAYLOAD_SESSION_META : KYMO_TRACE_PAYLOAD_DICTIONARY,
            feature->config.generation,
            session != 0u ? feature->config.metadata : feature->config.dictionary,
            session != 0u ? feature->config.metadata_length : feature->config.dictionary_length);
    }
    if (status == KYMO_OK)
    {
        status = KymoMux_ServiceAdmin(&feature->mux);
        if ((status == KYMO_OK) && (feature->mux.admin_pending == 0u) &&
            (feature->mux.pending == 0u))
        {
            ++feature->metadata_stage;
            if ((feature->metadata_stage == METADATA_COMPLETE) && (draining == 0u))
            {
                feature->mux.config.health_period_ticks = feature->config.health_period_ticks;
                feature->metadata_ready = 1u;
                feature->repeat_armed = 0u;
                if (feature->config.metadata_period_ticks != 0u)
                {
                    feature->metadata_completed_ticks =
                        feature->config.recorder.clock_ticks(feature->config.recorder.user);
                }
            }
        }
    }
    return status;
}

/*******************************************************************************
 * produceTrace
 ******************************************************************************/
static KymoStatus produceTrace(void *user, uint8_t source, uint32_t now, uint8_t draining)
{
    KymoEndpointTrace *feature = static_cast<KymoEndpointTrace *>(user);
    KymoStatus status = KYMO_OK;
    (void)source;
    (void)now;
    if ((draining == 0u) && (feature->metadata_stage == METADATA_COMPLETE) &&
        (feature->repeat_armed != 0u) && (feature->config.metadata_period_ticks != 0u) &&
        (feature->mux.pending == 0u) && (feature->mux.admin_pending == 0u))
    {
        uint64_t ticks = feature->config.recorder.clock_ticks(feature->config.recorder.user);
        if ((ticks - feature->metadata_completed_ticks) >= feature->config.metadata_period_ticks)
        {
            feature->metadata_stage = SESSION_METADATA;
            feature->mux.config.health_period_ticks = 0u;
        }
    }
    if (feature->metadata_stage != METADATA_COMPLETE)
    {
        status = produceMetadata(feature, draining);
    }
    else
    {
        if ((draining != 0u) && (feature->end_queued == 0u))
        {
            status = KymoMux_QueueSessionEnd(&feature->mux, feature->config.generation);
            if (status == KYMO_OK)
            {
                feature->end_queued = 1u;
            }
        }
        if ((status == KYMO_OK) || (status == KYMO_BUSY))
        {
            status = KymoMux_Service(&feature->mux);
            feature->repeat_armed = 1u;
            if ((draining != 0u) && (feature->end_queued == 0u) && (status == KYMO_IDLE))
            {
                status = KYMO_BUSY;
            }
        }
    }
    return status;
}

/*******************************************************************************
 * KymoEndpointTrace_Init
 ******************************************************************************/
KymoStatus KymoEndpointTrace_Init(KymoEndpoint *endpoint, KymoEndpointTrace *feature,
                                  const KymoEndpointTraceConfig *config)
{
    KymoEndpointFeature *binding = feature != nullptr ? &feature->binding : nullptr;
    KymoStatus status =
        kymoEndpointPrepare(endpoint, binding, KYMO_ENDPOINT_TRACE, KYMO_TRACE_FRAME_MAX, 0u, 0u);
    if (status == KYMO_OK)
    {
        status = KYMO_BAD_CONFIG;
        if ((config != nullptr) && (config->metadata != nullptr) &&
            (config->metadata_length != 0u) &&
            (config->metadata_length <= KYMO_MUX_ADMIN_RECORD_MAX - sizeof(uint32_t)) &&
            (config->dictionary != nullptr) && (config->dictionary_length != 0u) &&
            (config->dictionary_length <= KYMO_MUX_ADMIN_RECORD_MAX - sizeof(uint32_t)) &&
            (config->generation != 0u) && (config->recorder.session_id != 0u))
        {
            status = KymoTrace_Init(&feature->recorder, &config->recorder);
            if (status == KYMO_OK)
            {
                KymoMuxConfig mux = {};
                mux.recorder = &feature->recorder;
                mux.write = kymoEndpointCopy;
                mux.transport_user = endpoint;
                status = KymoMux_Init(&feature->mux, &mux);
                feature->config = *config;
            }
        }
        binding->user = feature;
        binding->sources = 1u;
        binding->produce = produceTrace;
        binding->stop = stopTrace;
    }
    return kymoEndpointFinish(endpoint, binding, status);
}

/*******************************************************************************
 * KymoEndpointTrace_Recorder
 ******************************************************************************/
KymoTraceRecorder *KymoEndpointTrace_Recorder(KymoEndpointTrace *feature)
{
    KymoTraceRecorder *recorder = nullptr;
    if ((feature != nullptr) && (feature->binding.endpoint != nullptr) &&
        ((feature->binding.endpoint->phase == KYMO_ENDPOINT_CONFIGURING) ||
         (feature->binding.endpoint->phase == KYMO_ENDPOINT_RUNNING)))
    {
        recorder = &feature->recorder;
    }
    return recorder;
}

/*******************************************************************************
 * KymoEndpointTrace_IsReady
 ******************************************************************************/
uint8_t KymoEndpointTrace_IsReady(const KymoEndpointTrace *feature)
{
    uint8_t ready = 0u;
    if ((feature != nullptr) && (feature->binding.endpoint != nullptr))
    {
        ready = (uint8_t)((feature->metadata_ready != 0u) &&
                          (feature->binding.endpoint->phase == KYMO_ENDPOINT_RUNNING));
    }
    return ready;
}
#endif
