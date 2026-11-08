/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file kymo_trace.cpp @brief Bounded per-core recording implementation. */
#include "kymo_trace.h"

#if KYMO_EFFECTIVE_TRACE
#include <string.h>

namespace {

/*******************************************************************************
 * recorderLock
 ******************************************************************************/
void recorderLock(KymoTraceRecorder *recorder)
{
    if (recorder->config.critical_enter != NULL) {
        recorder->config.critical_enter(recorder->config.user);
    }
}

/*******************************************************************************
 * recorderUnlock
 ******************************************************************************/
void recorderUnlock(KymoTraceRecorder *recorder)
{
    if (recorder->config.critical_exit != NULL) {
        recorder->config.critical_exit(recorder->config.user);
    }
}

/*******************************************************************************
 * activeCore
 ******************************************************************************/
uint8_t activeCore(KymoTraceRecorder *recorder)
{
    uint8_t result = 0u;
    if (recorder->config.core_id != NULL) {
        result = recorder->config.core_id(recorder->config.user);
    }
    if (result >= KYMO_TRACE_CORE_COUNT) {
        result = 0xffu;
    }
    return result;
}

/*******************************************************************************
 * findContext
 ******************************************************************************/
KymoTraceContextState *findContext(KymoTraceRecorder *recorder, uint32_t id,
                                   uint8_t create)
{
    KymoTraceContextState *found = NULL;
    for (uint16_t i = 0u; i < KYMO_TRACE_MAX_CONTEXTS; ++i) {
        if ((recorder->contexts[i].allocated != 0u) &&
            (recorder->contexts[i].id == id)) {
            found = &recorder->contexts[i];
            break;
        }
        if ((create != 0u) && (recorder->contexts[i].allocated == 0u) &&
            (found == NULL)) {
            found = &recorder->contexts[i];
        }
    }
    if ((found != NULL) && (found->allocated == 0u) && (id != 0u)) {
        memset(found, 0, sizeof(*found));
        found->id = id;
        found->allocated = 1u;
    } else if (id == 0u) {
        found = NULL;
    }
    return found;
}

/*******************************************************************************
 * appendEvent
 ******************************************************************************/
void appendEvent(KymoTraceRecorder *recorder, uint8_t core,
                 KymoTraceEvent *event)
{
    if (core < KYMO_TRACE_CORE_COUNT) {
        event->record_seq = recorder->record_seq[core]++;
        recorder->producer_next[core] = recorder->record_seq[core];
        if (recorder->counts[core] < KYMO_TRACE_EVENTS_PER_CORE) {
            uint16_t tail = (uint16_t)((recorder->heads[core] +
                recorder->counts[core]) & (KYMO_TRACE_EVENTS_PER_CORE - 1u));
            recorder->rings[core][tail] = *event;
            ++recorder->counts[core];
            if (recorder->counts[core] > recorder->high_water[core]) {
                recorder->high_water[core] = recorder->counts[core];
            }
        } else {
            ++recorder->drops[core];
        }
    }
}

/*******************************************************************************
 * eventDepth
 ******************************************************************************/
uint16_t eventDepth(uint32_t logicalDepth)
{
    uint16_t result = (logicalDepth > UINT16_MAX) ? UINT16_MAX :
        (uint16_t)logicalDepth;
    return result;
}

/*******************************************************************************
 * makeEvent
 ******************************************************************************/
void makeEvent(KymoTraceRecorder *recorder, uint32_t context, uint16_t symbol,
               uint8_t type, uint16_t depth, uint16_t flags, uint32_t arg,
               KymoTraceEvent *event, uint8_t core)
{
    memset(event, 0, sizeof(*event));
    event->timestamp_ticks = recorder->config.clock_ticks(recorder->config.user);
    event->context_id = context;
    event->symbol_id = symbol;
    event->depth = depth;
    event->event_type = type;
    event->core_id = core;
    event->flags = flags;
    event->arg = arg;
    KymoTraceContextState *state = findContext(recorder, context, 1u);
    if (state != NULL) {
        event->context_seq = state->sequence++;
        if (state->invalid != 0u) {
            event->flags = (uint16_t)(event->flags |
                KYMO_TRACE_FLAG_CONTEXT_INVALID);
        }
    } else {
        event->flags = (uint16_t)(event->flags |
            KYMO_TRACE_FLAG_CONTEXT_INVALID);
    }
}

/*******************************************************************************
 * normalizeCore
 ******************************************************************************/
uint8_t normalizeCore(KymoTraceRecorder *recorder, uint16_t *flags)
{
    uint8_t core = activeCore(recorder);
    if (core >= KYMO_TRACE_CORE_COUNT) {
        core = 0u;
        *flags = (uint16_t)(*flags | KYMO_TRACE_FLAG_CONTEXT_INVALID);
        ++recorder->errors[core];
    }
    return core;
}

} // namespace

/*******************************************************************************
 * KymoTrace_Init
 ******************************************************************************/
KymoStatus KymoTrace_Init(KymoTraceRecorder *recorder,
                          const KymoTraceConfig *config)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((recorder != NULL) && (config != NULL) &&
        (config->clock_ticks != NULL) &&
        ((config->critical_enter == NULL) == (config->critical_exit == NULL))) {
        memset(recorder, 0, sizeof(*recorder));
        recorder->config = *config;
        recorder->initialized = 1u;
        result = KYMO_OK;
    }
    return result;
}

/** @brief Stack address for ENTER/EXIT when sampling is compiled in.
 * @param[in,out] flags Event flags; gains KYMO_TRACE_FLAG_STACK_SAMPLE.
 * @return The stack address, or 0 when sampling is compiled out. */
/*******************************************************************************
 * stackSample
 ******************************************************************************/
static uint32_t stackSample(uint16_t *flags)
{
    uint32_t stack = 0u;
#if KYMO_EFFECTIVE_TRACE_STACK_POINTER
    *flags = (uint16_t)(*flags | KYMO_TRACE_FLAG_STACK_SAMPLE);
    stack = KYMO_TRACE_READ_STACK_POINTER();
#else
    (void)flags;
#endif
    return stack;
}

/*******************************************************************************
 * KymoTrace_Enter
 ******************************************************************************/
void KymoTrace_Enter(KymoTraceRecorder *recorder, uint32_t context,
                     uint16_t symbol)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = 0u;
        uint8_t core = normalizeCore(recorder, &flags);
        KymoTraceContextState *state = findContext(recorder, context, 1u);
        uint32_t before = 0u;
        if ((symbol == 0u) || (state == NULL)) {
            flags = (uint16_t)(flags | KYMO_TRACE_FLAG_CONTEXT_INVALID);
            ++recorder->errors[core];
            if (state != NULL) {
                state->invalid = 1u;
            }
        } else {
            before = state->depth;
            if (before < KYMO_TRACE_MAX_DEPTH) {
                state->symbols[before] = symbol;
            } else {
                state->invalid = 1u;
            }
            if (state->depth < UINT32_MAX) {
                ++state->depth;
            } else {
                state->invalid = 1u;
                state->depth_overflow = 1u;
                ++recorder->errors[core];
            }
            if ((before == 0u) && (state->invalid == 0u)) {
                flags = (uint16_t)(flags | KYMO_TRACE_FLAG_ROOT_BOUNDARY);
            }
        }
        KymoTraceEvent event;
        uint32_t stack = stackSample(&flags);
        makeEvent(recorder, context, symbol, KYMO_TRACE_EVENT_ENTER,
                  (state != NULL) ? eventDepth(state->depth) : 0u, flags, stack,
                  &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}

/*******************************************************************************
 * KymoTrace_Exit
 ******************************************************************************/
void KymoTrace_Exit(KymoTraceRecorder *recorder, uint32_t context,
                    uint16_t symbol)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = 0u;
        uint8_t core = normalizeCore(recorder, &flags);
        KymoTraceContextState *state = findContext(recorder, context, 0u);
        uint32_t depth = 0u;
        if ((symbol == 0u) || (state == NULL) || (state->depth == 0u)) {
            flags = (uint16_t)(flags | KYMO_TRACE_FLAG_CONTEXT_INVALID);
            ++recorder->errors[core];
            if (state != NULL) {
                state->invalid = 1u;
            }
        } else {
            depth = state->depth;
            if ((depth <= KYMO_TRACE_MAX_DEPTH) &&
                (state->symbols[depth - 1u] != symbol)) {
                flags = (uint16_t)(flags | KYMO_TRACE_FLAG_CONTEXT_INVALID);
                state->invalid = 1u;
                ++recorder->errors[core];
            } else if (depth > KYMO_TRACE_MAX_DEPTH) {
                state->invalid = 1u;
            }
            --state->depth;
        }
        KymoTraceEvent event;
        uint32_t stack = stackSample(&flags);
        makeEvent(recorder, context, symbol, KYMO_TRACE_EVENT_EXIT,
                  eventDepth(depth), flags, stack, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}

/*******************************************************************************
 * KymoTrace_Mark
 ******************************************************************************/
void KymoTrace_Mark(KymoTraceRecorder *recorder, uint32_t context,
                    uint16_t marker, uint32_t value)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_ARG_VALID;
        uint8_t core = normalizeCore(recorder, &flags);
        if (marker == 0u) {
            flags = (uint16_t)(flags | KYMO_TRACE_FLAG_CONTEXT_INVALID);
            ++recorder->errors[core];
        }
        KymoTraceEvent event;
        makeEvent(recorder, context, marker, KYMO_TRACE_EVENT_MARK, 0u, flags,
                  value, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}

#if KYMO_EFFECTIVE_TRACE_ISR
/*******************************************************************************
 * KymoTrace_IsrEnter
 ******************************************************************************/
void KymoTrace_IsrEnter(KymoTraceRecorder *recorder, uint32_t irqContext)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_ARG_VALID;
        uint8_t core = normalizeCore(recorder, &flags);
        uint16_t depth = recorder->isr_depth[core];
        uint32_t interrupted = 0u;
        if (depth > 0u) {
            interrupted = recorder->irq_contexts[core][depth - 1u];
        } else if (recorder->config.current_context != NULL) {
            interrupted = recorder->config.current_context(recorder->config.user);
        }
        if (depth < KYMO_TRACE_MAX_ISR_DEPTH) {
            recorder->interrupted_contexts[core][depth] = interrupted;
            recorder->irq_contexts[core][depth] = irqContext;
            ++recorder->isr_depth[core];
        } else {
            flags = (uint16_t)(flags | KYMO_TRACE_FLAG_CONTEXT_INVALID);
            ++recorder->errors[core];
        }
        KymoTraceEvent event;
        makeEvent(recorder, irqContext, 0u, KYMO_TRACE_EVENT_ISR_ENTER, 0u,
                  flags, interrupted, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}

/*******************************************************************************
 * KymoTrace_IsrExit
 ******************************************************************************/
void KymoTrace_IsrExit(KymoTraceRecorder *recorder, uint32_t irqContext)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_ARG_VALID;
        uint8_t core = normalizeCore(recorder, &flags);
        uint16_t depth = recorder->isr_depth[core];
        uint32_t restored = 0u;
        if ((depth > 0u) &&
            (recorder->irq_contexts[core][depth - 1u] == irqContext)) {
            --recorder->isr_depth[core];
            restored = recorder->interrupted_contexts[core][depth - 1u];
        } else {
            flags = (uint16_t)(flags | KYMO_TRACE_FLAG_CONTEXT_INVALID);
            ++recorder->errors[core];
        }
        KymoTraceEvent event;
        makeEvent(recorder, irqContext, 0u, KYMO_TRACE_EVENT_ISR_EXIT, 0u,
                  flags, restored, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER
/*******************************************************************************
 * KymoTrace_Scheduler
 ******************************************************************************/
void KymoTrace_Scheduler(KymoTraceRecorder *recorder, uint32_t context,
                         uint32_t task)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_ARG_VALID;
        uint8_t core = normalizeCore(recorder, &flags);
        KymoTraceEvent event;
        makeEvent(recorder, context, 0u, KYMO_TRACE_EVENT_TASK_SWITCH, 0u,
                  flags, task, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}

/*******************************************************************************
 * KymoTrace_TaskCreate
 ******************************************************************************/
void KymoTrace_TaskCreate(KymoTraceRecorder *recorder, uint32_t context,
                          uint32_t generation)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_ARG_VALID;
        uint8_t core = normalizeCore(recorder, &flags);
        KymoTraceEvent event;
        makeEvent(recorder, context, 0u, KYMO_TRACE_EVENT_TASK_CREATE, 0u,
                  flags, generation, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}

/*******************************************************************************
 * KymoTrace_TaskEnd
 ******************************************************************************/
void KymoTrace_TaskEnd(KymoTraceRecorder *recorder, uint32_t context,
                       uint32_t generation)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_ARG_VALID;
        uint8_t core = normalizeCore(recorder, &flags);
        KymoTraceEvent event;
        makeEvent(recorder, context, 0u, KYMO_TRACE_EVENT_TASK_END, 0u,
                  flags, generation, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}

/*******************************************************************************
 * KymoTrace_CoreState
 ******************************************************************************/
void KymoTrace_CoreState(KymoTraceRecorder *recorder, uint32_t context,
                         uint8_t state, uint8_t recovery)
{
    if ((recorder != NULL) && (recorder->initialized != 0u) &&
        (state <= 2u) && (recovery <= 1u)) {
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_ARG_VALID;
        uint8_t core = normalizeCore(recorder, &flags);
        if (recovery != 0u) {
            flags = (uint16_t)(flags | KYMO_TRACE_FLAG_RECOVERY);
        }
        KymoTraceEvent event;
        makeEvent(recorder, context, 0u, KYMO_TRACE_EVENT_CORE_STATE, 0u,
                  flags, state, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}
#endif
#if KYMO_EFFECTIVE_TRACE_FLOW
/*******************************************************************************
 * KymoTrace_FlowSend
 ******************************************************************************/
void KymoTrace_FlowSend(KymoTraceRecorder *recorder, uint32_t context,
                        uint16_t symbol, uint32_t correlation)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_ARG_VALID;
        uint8_t core = normalizeCore(recorder, &flags);
        KymoTraceEvent event;
        makeEvent(recorder, context, symbol, KYMO_TRACE_EVENT_FLOW_SEND, 0u,
                  flags, correlation, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}

/*******************************************************************************
 * KymoTrace_FlowReceive
 ******************************************************************************/
void KymoTrace_FlowReceive(KymoTraceRecorder *recorder, uint32_t context,
                           uint16_t symbol, uint32_t correlation)
{
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_ARG_VALID;
        uint8_t core = normalizeCore(recorder, &flags);
        KymoTraceEvent event;
        makeEvent(recorder, context, symbol, KYMO_TRACE_EVENT_FLOW_RECEIVE, 0u,
                  flags, correlation, &event, core);
        appendEvent(recorder, core, &event);
        recorderUnlock(recorder);
    }
}
#endif

/*******************************************************************************
 * KymoTrace_ContextReset
 ******************************************************************************/
KymoStatus KymoTrace_ContextReset(KymoTraceRecorder *recorder, uint32_t context,
                                  uint32_t generation)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((recorder != NULL) && (recorder->initialized != 0u)) {
        result = KYMO_REJECTED;
        recorderLock(recorder);
        uint16_t flags = KYMO_TRACE_FLAG_RECOVERY;
        uint8_t core = normalizeCore(recorder, &flags);
        KymoTraceContextState *state = findContext(recorder, context, 1u);
        if ((state != NULL) && (state->depth == 0u) &&
            (state->depth_overflow == 0u)) {
            state->invalid = 0u;
            KymoTraceEvent event;
            makeEvent(recorder, context, 0u, KYMO_TRACE_EVENT_CONTEXT_RESET,
                      0u, flags, generation, &event, core);
            appendEvent(recorder, core, &event);
            result = KYMO_OK;
        }
        recorderUnlock(recorder);
    }
    return result;
}

/*******************************************************************************
 * KymoTrace_Pop
 ******************************************************************************/
KymoStatus KymoTrace_Pop(KymoTraceRecorder *recorder, KymoTraceEvent *event)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((recorder != NULL) && (event != NULL) &&
        (recorder->initialized != 0u)) {
        result = KYMO_IDLE;
        recorderLock(recorder);
        for (uint8_t offset = 0u; (offset < KYMO_TRACE_CORE_COUNT) &&
             (result == KYMO_IDLE); ++offset) {
            uint8_t core = (uint8_t)((recorder->pop_core + offset) %
                KYMO_TRACE_CORE_COUNT);
            if (recorder->counts[core] > 0u) {
                *event = recorder->rings[core][recorder->heads[core]];
                recorder->heads[core] = (uint16_t)((recorder->heads[core] +
                    1u) & (KYMO_TRACE_EVENTS_PER_CORE - 1u));
                --recorder->counts[core];
                recorder->settled_through[core] = event->record_seq + 1u;
                recorder->pop_core = (uint8_t)((core + 1u) %
                    KYMO_TRACE_CORE_COUNT);
                result = KYMO_OK;
            }
        }
        recorderUnlock(recorder);
    }
    return result;
}

/*******************************************************************************
 * KymoTrace_DropCount
 ******************************************************************************/
uint32_t KymoTrace_DropCount(KymoTraceRecorder *recorder, uint8_t core)
{
    uint32_t result = 0u;
    if ((recorder != NULL) && (recorder->initialized != 0u) &&
        (core < KYMO_TRACE_CORE_COUNT)) {
        recorderLock(recorder);
        result = recorder->drops[core];
        recorderUnlock(recorder);
    }
    return result;
}
#endif /* KYMO_EFFECTIVE_TRACE */
