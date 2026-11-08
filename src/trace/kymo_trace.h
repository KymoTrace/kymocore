/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */

/** @file kymo_trace.h
 * @brief Fixed-storage execution trace recorder.
 * @details Hook calls never allocate or transport; the mux (kymo_mux.h) drains the
 * recorder. Concurrent callers require paired critical callbacks that serialize all
 * recorder accesses; these callbacks are not declared ISR-safe by this portable
 * library. Resource sizes (KYMO_TRACE_*) are set in kymo_config.h. Persistent
 * recorder storage is caller-owned. Available with KYMO_ENABLE_TRACE=1.
 * @par Usage
 * @code{.c}
 * KymoTraceConfig config = {0};
 * config.clock_ticks = read_cycle_counter;   // only required callback
 * static KymoTraceRecorder recorder;
 * if (KymoTrace_Init(&recorder, &config) == KYMO_OK) { KYMO_TRACE_ENTER(&recorder, 1u, 7u); }
 * @endcode
 */
#ifndef KYMO_TRACE_H
#define KYMO_TRACE_H

#include "../kymo_types.h"

#if KYMO_EFFECTIVE_TRACE
#include "kymo_trace_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Event flag: first scope of a context (call-tree root). */
#define KYMO_TRACE_FLAG_ROOT_BOUNDARY 0x0001u
/** @brief Event flag: context state is unreliable (mismatch, overflow, unknown core). */
#define KYMO_TRACE_FLAG_CONTEXT_INVALID 0x0002u
/** @brief Event flag: KymoTraceEvent::arg carries a value. */
#define KYMO_TRACE_FLAG_ARG_VALID 0x0004u
/** @brief Event flag: trusted recovery checkpoint. */
#define KYMO_TRACE_FLAG_RECOVERY 0x0008u
/** @brief Event flag: ENTER/EXIT arg holds the stack address (KYMO_ENABLE_TRACE_STACK_POINTER). */
#define KYMO_TRACE_FLAG_STACK_SAMPLE 0x0010u

/** @brief Recorder clock and synchronization bindings.
 * @details Only clock_ticks is required. When critical callbacks are absent the
 * application serializes every recorder access. Supplied callbacks are not presumed
 * ISR-safe. Start from a zero-initialized object and assign fields by name.
 */
typedef struct KymoTraceConfig {
    /** @brief Required: read monotonically increasing hardware ticks. */
    uint64_t (*clock_ticks)(void *user);

    /** @brief Optional: return a core ID in 0..KYMO_TRACE_CORE_COUNT-1; NULL means core 0. */
    uint8_t (*core_id)(void *user);

    /** @brief Optional: return the active context ID for ISR entry; NULL means unknown (0). */
    uint32_t (*current_context)(void *user);

    /** @brief Enter the application's recorder-wide critical section. */
    void (*critical_enter)(void *user);

    /** @brief Exit the application's recorder-wide critical section. */
    void (*critical_exit)(void *user);

    /** @brief User pointer passed to all callbacks. */
    void *user;

    /** @brief Session identifier copied into mux envelope configuration. */
    uint64_t session_id;

    /** @brief Clock-domain identifier serialized in event batches. */
    uint16_t clock_domain_id;
} KymoTraceConfig;

/** @brief Shared scope state keyed by application context ID. */
typedef struct KymoTraceContextState {
    uint32_t id;
    uint32_t sequence;
    uint32_t depth;
    uint8_t allocated;
    uint8_t invalid;
    uint8_t depth_overflow;
    uint16_t symbols[KYMO_TRACE_MAX_DEPTH];
} KymoTraceContextState;

/** @brief Fixed-storage event recorder; the application owns this object. */
typedef struct KymoTraceRecorder {
    KymoTraceConfig config;
    KymoTraceEvent rings[KYMO_TRACE_CORE_COUNT][KYMO_TRACE_EVENTS_PER_CORE];
    KymoTraceContextState contexts[KYMO_TRACE_MAX_CONTEXTS];
    uint32_t record_seq[KYMO_TRACE_CORE_COUNT];
    uint32_t drops[KYMO_TRACE_CORE_COUNT];
    uint32_t errors[KYMO_TRACE_CORE_COUNT];
    uint32_t producer_next[KYMO_TRACE_CORE_COUNT];
    uint32_t settled_through[KYMO_TRACE_CORE_COUNT];
    uint16_t high_water[KYMO_TRACE_CORE_COUNT];
    uint16_t heads[KYMO_TRACE_CORE_COUNT];
    uint16_t counts[KYMO_TRACE_CORE_COUNT];

#if KYMO_EFFECTIVE_TRACE_ISR
    uint16_t isr_depth[KYMO_TRACE_CORE_COUNT];
    uint32_t interrupted_contexts[KYMO_TRACE_CORE_COUNT][KYMO_TRACE_MAX_ISR_DEPTH];
    uint32_t irq_contexts[KYMO_TRACE_CORE_COUNT][KYMO_TRACE_MAX_ISR_DEPTH];
#endif
    uint8_t initialized;
    uint8_t pop_core;
} KymoTraceRecorder;

/** @brief Initialize persistent recorder storage.
 * @param[out] recorder Caller-owned object to initialize.
 * @param[in] config Required time and context callbacks and synchronization contract.
 * @return KYMO_OK on success; KYMO_BAD_CONFIG on invalid pointers or callback pairing.
 */
KymoStatus KymoTrace_Init(KymoTraceRecorder *recorder, const KymoTraceConfig *config);

/** @brief Record a function or scope entry; bounded and allocation-free.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id Active application context ID.
 * @param[in] symbol_id Registered symbol ID; zero is invalid.
 */
void KymoTrace_Enter(KymoTraceRecorder *recorder, uint32_t context_id, uint16_t symbol_id);

/** @brief Record scope exit and validate it against the context stack.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id Active application context ID.
 * @param[in] symbol_id Symbol ID that must match the top scope.
 */
void KymoTrace_Exit(KymoTraceRecorder *recorder, uint32_t context_id, uint16_t symbol_id);

/** @brief Record a marker and its value.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id Active application context ID.
 * @param[in] marker_id Registered marker symbol ID.
 * @param[in] value Application-defined marker value.
 */
void KymoTrace_Mark(
    KymoTraceRecorder *recorder,
    uint32_t context_id,
    uint16_t marker_id,
    uint32_t value);

#if KYMO_EFFECTIVE_TRACE_ISR

/** @brief Record ISR entry and save interrupted-context identity.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] irq_context Registered ISR context ID.
 */
void KymoTrace_IsrEnter(KymoTraceRecorder *recorder, uint32_t irq_context);

/** @brief Record ISR exit and restore the interrupted-context identity.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] irq_context Must match the current ISR context ID.
 */
void KymoTrace_IsrExit(KymoTraceRecorder *recorder, uint32_t irq_context);
#endif

#if KYMO_EFFECTIVE_TRACE_SCHEDULER

/** @brief Record an explicit task switch; no scheduler hook is installed.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id Outgoing context ID, or zero if unknown/idle.
 * @param[in] task_id Incoming context ID.
 */
void KymoTrace_Scheduler(KymoTraceRecorder *recorder, uint32_t context_id, uint32_t task_id);

/** @brief Record task creation.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id New task context ID.
 * @param[in] generation Task generation stored in the event argument.
 */
void KymoTrace_TaskCreate(
    KymoTraceRecorder *recorder,
    uint32_t context_id,
    uint32_t generation);

/** @brief Record task termination.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id Terminated task context ID.
 * @param[in] generation Task generation stored in the event argument.
 */
void KymoTrace_TaskEnd(KymoTraceRecorder *recorder, uint32_t context_id, uint32_t generation);

/** @brief Record a per-core running-context checkpoint.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id Running context ID, or zero when unknown/idle.
 * @param[in] state 0 unknown, 1 running task, or 2 idle.
 * @param[in] recovery Nonzero to mark a trusted recovery checkpoint.
 */
void KymoTrace_CoreState(
    KymoTraceRecorder *recorder,
    uint32_t context_id,
    uint8_t state,
    uint8_t recovery);
#endif

#if KYMO_EFFECTIVE_TRACE_FLOW

/** @brief Record a flow correlation send.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id Sending context ID.
 * @param[in] symbol_id Optional registered operation symbol ID (zero if absent).
 * @param[in] correlation_id Application-defined correlation value.
 */
void KymoTrace_FlowSend(
    KymoTraceRecorder *recorder,
    uint32_t context_id,
    uint16_t symbol_id,
    uint32_t correlation_id);

/** @brief Record a flow correlation receive.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id Receiving context ID.
 * @param[in] symbol_id Optional registered operation symbol ID (zero if absent).
 * @param[in] correlation_id Application-defined correlation value.
 */
void KymoTrace_FlowReceive(
    KymoTraceRecorder *recorder,
    uint32_t context_id,
    uint16_t symbol_id,
    uint32_t correlation_id);
#endif

/** @brief Emit a context recovery event when the caller confirms depth is zero.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] context_id Context to recover.
 * @param[in] generation New application-supplied context generation.
 * @return KYMO_OK if recorded; KYMO_REJECTED while the context has open scopes or
 *         no context slot is free; KYMO_BAD_CONFIG for an uninitialized recorder.
 */
KymoStatus KymoTrace_ContextReset(
    KymoTraceRecorder *recorder,
    uint32_t context_id,
    uint32_t generation);

/** @brief Pop one event in round-robin core order.
 * @param[in,out] recorder Initialized recorder.
 * @param[out] event Receives the next event if available.
 * @return KYMO_OK if copied; KYMO_IDLE if no event is queued; KYMO_BAD_CONFIG for
 *         invalid arguments or an uninitialized recorder.
 */
KymoStatus KymoTrace_Pop(KymoTraceRecorder *recorder, KymoTraceEvent *event);

/** @brief Read cumulative dropped events for one core under recorder locking.
 * @param[in,out] recorder Initialized recorder.
 * @param[in] core_id Core index.
 * @return Cumulative overflow drops, or zero for an invalid core/recorder.
 */
uint32_t KymoTrace_DropCount(KymoTraceRecorder *recorder, uint8_t core_id);

#ifdef __cplusplus
}

/** @brief C++ RAII scope that emits Enter on construction and Exit on destruction.
 * @details Copy and move operations are disabled to prevent duplicate exits.
 */
class KymoTraceScope {
public:

    /** @brief Begin a measured scope.
     * @param[in,out] recorder Initialized recorder.
     * @param[in] context Context ID to associate with the scope.
     * @param[in] symbol Registered symbol ID.
     */
    KymoTraceScope(KymoTraceRecorder *recorder, uint32_t context, uint16_t symbol)
        : m_recorder(recorder), m_context(context), m_symbol(symbol)
    {
        KymoTrace_Enter(m_recorder, m_context, m_symbol);
    }

    /** @brief End the measured scope. */
    ~KymoTraceScope()
    {
        KymoTrace_Exit(m_recorder, m_context, m_symbol);
    }
    KymoTraceScope(const KymoTraceScope &) = delete;
    KymoTraceScope &operator=(const KymoTraceScope &) = delete;

private:
    KymoTraceRecorder *m_recorder;
    uint32_t m_context;
    uint16_t m_symbol;
};
#endif /* __cplusplus */
#endif /* KYMO_EFFECTIVE_TRACE */
#endif /* KYMO_TRACE_H */
