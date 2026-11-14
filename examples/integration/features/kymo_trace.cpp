/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_trace.cpp
 * @brief Trace-specific bindings and complete initialization.
 */
#include "kymo_trace.h"
#include "../detail/kymo_internal.h"
static KymoCoreEndpointTrace sFeature;
#include "../kymo_port.h"
#include "../kymo_trace_records.h"
static const uint64_t TRACE_SESSION_ID = 1u;
static const uint16_t TRACE_CLOCK_DOMAIN_ID = 0u;
static const uint32_t TRACE_CONTEXT_ID = 1u;
static const uint16_t TRACE_MARKER_ID = 1u;
static const uint32_t TRACE_RECORD_GENERATION = 1u;
static const uint32_t TRACE_MARKER_PERIOD_CYCLES = 200u;

/** @brief Fixed trace resources, callbacks and feature settings. */
static const KymoCoreEndpointTraceConfig sTraceConfig = {{examplePortClockTicks, nullptr, nullptr,
                                                          nullptr, nullptr, nullptr,
                                                          TRACE_SESSION_ID, TRACE_CLOCK_DOMAIN_ID},
                                                         exampleTraceMetadata,
                                                         sizeof(exampleTraceMetadata) - 1u,
                                                         exampleTraceDictionary,
                                                         sizeof(exampleTraceDictionary) - 1u,
                                                         TRACE_RECORD_GENERATION,
                                                         0u,
                                                         0u};

/****************************************************************************************************
 * exampleTraceConfig
 ***************************************************************************************************/
const KymoCoreEndpointTraceConfig &exampleTraceConfig(void)
{
    return sTraceConfig;
}

/****************************************************************************************************
 * KymoApp_TraceInit
 ***************************************************************************************************/
KymoStatus KymoApp_TraceInit(const KymoCoreEndpointTraceConfig *config)
{
    return KymoCoreEndpointTrace_init(exampleEndpoint(), &sFeature, config);
}

/****************************************************************************************************
 * exampleTraceCycle
 ***************************************************************************************************/
void exampleTraceCycle(uint32_t cycle)
{
    if ((KymoCoreEndpointTrace_isReady(&sFeature) != 0u) &&
        ((cycle % TRACE_MARKER_PERIOD_CYCLES) == 0u))
    {
        KYMO_ENDPOINT_TRACE_MARK(&sFeature, TRACE_CONTEXT_ID, TRACE_MARKER_ID, cycle);
    }
}
