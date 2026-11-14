/* SPDX-License-Identifier: MIT */
/** @file kymo_core_trace.cpp
 * @brief Default trace ownership and instrumentation routing.
 */
#include "../../kymo_core_trace.h"
#if KYMO_EFFECTIVE_CORE && KYMO_EFFECTIVE_TRACE
#include "kymo_core_endpoint_trace.h"
#include "../../core/detail/kymo_core_internal.h"
static KymoCoreEndpointTrace defaultTrace = {};

/****************************************************************************************************
 * KymoCoreTrace_init
 ***************************************************************************************************/
/* Translate application configuration and register the recorder. */
KymoStatus KymoCoreTrace_init(const KymoCoreTraceConfig *pConfig)
{
    KymoCoreEndpointTraceConfig internalConfig = {};
    KymoStatus status = KYMO_BAD_CONFIG;
    if (pConfig != nullptr)
    {
        internalConfig.recorder.clock_ticks = pConfig->clockTicksCbk;
        internalConfig.recorder.core_id = pConfig->coreIdCbk;
        internalConfig.recorder.current_context = pConfig->currentContextCbk;
        internalConfig.recorder.critical_enter = pConfig->enterCriticalCbk;
        internalConfig.recorder.critical_exit = pConfig->exitCriticalCbk;
        internalConfig.recorder.user = pConfig->pContext;
        internalConfig.recorder.session_id = pConfig->sessionId;
        internalConfig.recorder.clock_domain_id = pConfig->clockDomainId;
        internalConfig.metadata = pConfig->pMetadataAv;
        internalConfig.metadata_length = pConfig->metadataSizeBytes;
        internalConfig.dictionary = pConfig->pDictionaryAv;
        internalConfig.dictionary_length = pConfig->dictionarySizeBytes;
        internalConfig.generation = pConfig->generation;
        internalConfig.health_period_ticks = pConfig->healthPeriodTicks;
        internalConfig.metadata_period_ticks = pConfig->metadataPeriodTicks;
    }
    status = KymoCoreEndpointTrace_init(KymoCore_getEndpoint(), &defaultTrace,
                                        pConfig != nullptr ? &internalConfig : nullptr);
    return status;
}

/****************************************************************************************************
 * KymoCoreTrace_enter
 ***************************************************************************************************/
/* Record function entry; symbolId must be nonzero. */
void KymoCoreTrace_enter(uint32_t contextId, uint16_t symbolId)
{
    KymoTrace_enter(KymoCoreEndpointTrace_recorder(&defaultTrace), contextId, symbolId);
}

/****************************************************************************************************
 * KymoCoreTrace_exit
 ***************************************************************************************************/
/* Record matching function exit, including exits on error paths. */
void KymoCoreTrace_exit(uint32_t contextId, uint16_t symbolId)
{
    KymoTrace_exit(KymoCoreEndpointTrace_recorder(&defaultTrace), contextId, symbolId);
}

/****************************************************************************************************
 * KymoCoreTrace_mark
 ***************************************************************************************************/
/* Record an application marker with an unsigned value. */
void KymoCoreTrace_mark(uint32_t contextId, uint16_t markerId, uint32_t value)
{
    KymoTrace_mark(KymoCoreEndpointTrace_recorder(&defaultTrace), contextId, markerId, value);
}
#if KYMO_EFFECTIVE_TRACE_ISR

/****************************************************************************************************
 * KymoCoreTrace_isrEnter
 ***************************************************************************************************/
/* Record interrupt entry; synchronization must support this context. */
void KymoCoreTrace_isrEnter(uint32_t irqContextId)
{
    KymoTrace_isrEnter(KymoCoreEndpointTrace_recorder(&defaultTrace), irqContextId);
}
#endif
#if KYMO_EFFECTIVE_TRACE_ISR

/****************************************************************************************************
 * KymoCoreTrace_isrExit
 ***************************************************************************************************/
/* Record interrupt exit matching the most recent entry. */
void KymoCoreTrace_isrExit(uint32_t irqContextId)
{
    KymoTrace_isrExit(KymoCoreEndpointTrace_recorder(&defaultTrace), irqContextId);
}
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER

/****************************************************************************************************
 * KymoCoreTrace_scheduler
 ***************************************************************************************************/
/* Record a switch to the selected task. */
void KymoCoreTrace_scheduler(uint32_t contextId, uint32_t taskId)
{
    KymoTrace_scheduler(KymoCoreEndpointTrace_recorder(&defaultTrace), contextId, taskId);
}
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER

/****************************************************************************************************
 * KymoCoreTrace_taskCreate
 ***************************************************************************************************/
/* Record creation of a context generation. */
void KymoCoreTrace_taskCreate(uint32_t contextId, uint32_t generation)
{
    KymoTrace_taskCreate(KymoCoreEndpointTrace_recorder(&defaultTrace), contextId, generation);
}
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER

/****************************************************************************************************
 * KymoCoreTrace_taskEnd
 ***************************************************************************************************/
/* Record completion of a context generation. */
void KymoCoreTrace_taskEnd(uint32_t contextId, uint32_t generation)
{
    KymoTrace_taskEnd(KymoCoreEndpointTrace_recorder(&defaultTrace), contextId, generation);
}
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER

/****************************************************************************************************
 * KymoCoreTrace_coreState
 ***************************************************************************************************/
/* Record core state: zero unknown, one running task, two idle; recovery is zero or one. */
void KymoCoreTrace_coreState(uint32_t contextId, uint8_t state, uint8_t isRecovery)
{
    KymoTrace_coreState(KymoCoreEndpointTrace_recorder(&defaultTrace), contextId, state,
                        isRecovery);
}
#endif
#if KYMO_EFFECTIVE_TRACE_FLOW

/****************************************************************************************************
 * KymoCoreTrace_flowSend
 ***************************************************************************************************/
/* Record the send side of a correlated operation. */
void KymoCoreTrace_flowSend(uint32_t contextId, uint16_t symbolId, uint32_t correlationId)
{
    KymoTrace_flowSend(KymoCoreEndpointTrace_recorder(&defaultTrace), contextId, symbolId,
                       correlationId);
}
#endif
#if KYMO_EFFECTIVE_TRACE_FLOW

/****************************************************************************************************
 * KymoCoreTrace_flowReceive
 ***************************************************************************************************/
/* Record the receive side of a correlated operation. */
void KymoCoreTrace_flowReceive(uint32_t contextId, uint16_t symbolId, uint32_t correlationId)
{
    KymoTrace_flowReceive(KymoCoreEndpointTrace_recorder(&defaultTrace), contextId, symbolId,
                          correlationId);
}
#endif

/****************************************************************************************************
 * KymoCoreTrace_contextReset
 ***************************************************************************************************/
/* Reset an inactive, fully unwound context to a new generation. */
KymoStatus KymoCoreTrace_contextReset(uint32_t contextId, uint32_t generation)
{
    KymoStatus result = KymoTrace_contextReset(KymoCoreEndpointTrace_recorder(&defaultTrace),
                                               contextId, generation);
    return result;
}

/****************************************************************************************************
 * KymoCoreTrace_dropCount
 ***************************************************************************************************/
/* Read the dropped-event count for one core. */
uint32_t KymoCoreTrace_dropCount(uint8_t coreId)
{
    uint32_t result = KymoTrace_dropCount(KymoCoreEndpointTrace_recorder(&defaultTrace), coreId);
    return result;
}

/****************************************************************************************************
 * KymoCoreTrace_isReady
 ***************************************************************************************************/
/* Report metadata readiness without exposing recorder state. */
uint8_t KymoCoreTrace_isReady(void)
{
    uint8_t isReady = KymoCoreEndpointTrace_isReady(&defaultTrace);
    return isReady;
}
#endif
