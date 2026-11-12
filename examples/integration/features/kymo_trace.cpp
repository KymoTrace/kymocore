/* SPDX-License-Identifier: MIT */
/** @file kymo_trace.cpp
 * @brief Trace-specific bindings and complete initialization.
 */
#include "kymo_trace.h"
#include "../detail/kymo_internal.h"
static KymoEndpointTrace sFeature;
#include "../kymo_port.h"
#include "../kymo_trace_records.h"
static const uint64_t TRACE_SESSION_ID = 1u;
static const uint16_t TRACE_CLOCK_DOMAIN_ID = 0u;
static const uint32_t TRACE_CONTEXT_ID = 1u;
static const uint16_t TRACE_MARKER_ID = 1u;
static const uint32_t TRACE_RECORD_GENERATION = 1u;
static const uint32_t TRACE_MARKER_PERIOD_CYCLES = 200u;
/*******************************************************************************
 * exampleTraceConfig
 ******************************************************************************/
KymoEndpointTraceConfig exampleTraceConfig(void)
{
    KymoEndpointTraceConfig config = {};
    config.recorder.clock_ticks = examplePortClockTicks;
    config.recorder.session_id = TRACE_SESSION_ID;
    config.recorder.clock_domain_id = TRACE_CLOCK_DOMAIN_ID;
    config.metadata = exampleTraceMetadata;
    config.metadata_length = sizeof(exampleTraceMetadata) - 1u;
    config.dictionary = exampleTraceDictionary;
    config.dictionary_length = sizeof(exampleTraceDictionary) - 1u;
    config.generation = TRACE_RECORD_GENERATION;
    return config;
}
/*******************************************************************************
 * KymoApp_TraceInit
 ******************************************************************************/
KymoStatus KymoApp_TraceInit(const KymoEndpointTraceConfig *config)
{
    return KymoEndpointTrace_Init(exampleEndpoint(), &sFeature, config);
}
/*******************************************************************************
 * exampleTraceCycle
 ******************************************************************************/
void exampleTraceCycle(uint32_t cycle)
{
    if ((KymoEndpointTrace_IsReady(&sFeature) != 0u) &&
        ((cycle % TRACE_MARKER_PERIOD_CYCLES) == 0u))
    {
        KYMO_ENDPOINT_TRACE_MARK(&sFeature, TRACE_CONTEXT_ID, TRACE_MARKER_ID, cycle);
    }
}
