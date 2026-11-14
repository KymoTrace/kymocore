/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_trace.h
 * @brief Application trace setup, instrumentation and diagnostics.
 * @details Core Main sends recorded events and metadata. Hooks only record events;
 * call them at the desired locations. Before Init and after a Core fault they are
 * inert. Serialize Core lifecycle with hooks. Lock callbacks protect recording,
 * not Core lifecycle; ISR use requires appropriate locks and nonblocking callbacks.
 * @par Usage Include this header, initialize Core and Trace, then enter/exit scopes.
 */
#ifndef KYMO_CORE_TRACE_H
#define KYMO_CORE_TRACE_H
#include "kymo_types.h"
#if KYMO_EFFECTIVE_TRACE || defined(DOXYGEN)

/** @brief Minimum Core TX capacity in bytes when Trace is initialized. */
#define KYMO_TRACE_FRAME_MAX 240u
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Read recorder ticks.
     * @param[in,out] pContext Application context supplied in the trace configuration.
     * @return Current clock or identifier value.
     * @par Usage Supply a bounded implementation suitable for every recording context.
     */
    typedef uint64_t (*KymoTraceClockTicksFn)(void *pContext);

    /**
     * @brief Read the current core index, below KYMO_TRACE_CORE_COUNT.
     * @param[in,out] pContext Application context supplied in the trace configuration.
     * @return Current clock or identifier value.
     * @par Usage Supply a bounded implementation suitable for every recording context.
     */
    typedef uint8_t (*KymoTraceCoreIdFn)(void *pContext);

    /**
     * @brief Read the interrupted context ID for ISR entry; zero means unknown.
     * @param[in,out] pContext Application context supplied in the trace configuration.
     * @return Current clock or identifier value.
     * @par Usage Supply a bounded implementation suitable for every recording context.
     */
    typedef uint32_t (*KymoTraceCurrentContextFn)(void *pContext);

    /**
     * @brief Enter or exit the application synchronization primitive.
     * @param[in,out] pContext Application context supplied in the trace configuration.
     * @par Usage Supply a bounded implementation suitable for every recording context.
     */
    typedef void (*KymoTraceCriticalFn)(void *pContext);

    /**
     * @brief Trace-only configuration for the default Core.
     * @details Data and contexts remain application-owned and must outlive Core.
     * @par Usage Assign callbacks first, then session metadata and intervals.
     */
    typedef struct
    {
        /** @brief Required monotonically increasing clock in recorder ticks. */
        KymoTraceClockTicksFn clockTicksCbk;

        /** @brief Optional core ID reader; NULL selects core zero. */
        KymoTraceCoreIdFn coreIdCbk;

        /** @brief Interrupted-context reader for ISR entry; NULL reports unknown (zero). */
        KymoTraceCurrentContextFn currentContextCbk;

        /** @brief Optional lock callback; provide together with exitCriticalCbk. */
        KymoTraceCriticalFn enterCriticalCbk;

        /** @brief Matching unlock callback; locking must cover every recording context. */
        KymoTraceCriticalFn exitCriticalCbk;

        /** @brief Shared callback context; may be NULL. */
        void *pContext;

        /** @brief Nonzero session identifier matching metadata. */
        uint64_t sessionId;

        /** @brief Clock domain identifier matching metadata. */
        uint16_t clockDomainId;

        /** @brief Persistent canonical session-metadata JSON, without trailing CRC. */
        const uint8_t *pMetadataAv;

        /** @brief Metadata size excluding the string terminator. */
        size_t metadataSizeBytes;

        /** @brief Persistent canonical symbol-dictionary JSON, without trailing CRC. */
        const uint8_t *pDictionaryAv;

        /** @brief Dictionary size excluding the string terminator. */
        size_t dictionarySizeBytes;

        /** @brief Nonzero metadata generation. */
        uint32_t generation;

        /** @brief Health reporting interval; zero disables it. */
        uint32_t healthPeriodTicks;

        /** @brief Metadata repeat interval after completion; zero sends only at startup. */
        uint32_t metadataPeriodTicks;
    } KymoCoreTraceConfig;
#if KYMO_EFFECTIVE_CORE || defined(DOXYGEN)

    /**
     * @brief Initialize Trace and register it in Core Main.
     * @details Call once after Core Init and before the first Main. Setup errors latch
     * a Core fault; late calls are rejected without changing a running Core.
     * @param[in] pConfig Trace settings; NULL is invalid.
     * @return OK, BAD_CONFIG, REJECTED, or the existing Core error.
     * @par Usage Check success before calling instrumentation hooks. Not ISR-safe.
     */
    KymoStatus KymoCoreTrace_init(const KymoCoreTraceConfig *pConfig);

    /**
     * @brief Record function entry; symbolId must be nonzero.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] symbolId Nonzero generated symbol identifier.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_enter(uint32_t contextId, uint16_t symbolId);

    /**
     * @brief Record matching function exit, including exits on error paths.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] symbolId Nonzero generated symbol identifier.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_exit(uint32_t contextId, uint16_t symbolId);

    /**
     * @brief Record an application marker with an unsigned value.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] markerId Nonzero generated marker identifier.
     * @param[in] value Application value attached to the marker.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_mark(uint32_t contextId, uint16_t markerId, uint32_t value);
#if KYMO_EFFECTIVE_TRACE_ISR || defined(DOXYGEN)

    /**
     * @brief Record interrupt entry; synchronization must support this context.
     * @param[in] irqContextId Interrupt context identifier.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_isrEnter(uint32_t irqContextId);
#endif
#if KYMO_EFFECTIVE_TRACE_ISR || defined(DOXYGEN)

    /**
     * @brief Record interrupt exit matching the most recent entry.
     * @param[in] irqContextId Interrupt context identifier.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_isrExit(uint32_t irqContextId);
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER || defined(DOXYGEN)

    /**
     * @brief Record a switch to the selected task.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] taskId Identifier of the selected task.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_scheduler(uint32_t contextId, uint32_t taskId);
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER || defined(DOXYGEN)

    /**
     * @brief Record creation of a context generation.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] generation Application context generation.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_taskCreate(uint32_t contextId, uint32_t generation);
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER || defined(DOXYGEN)

    /**
     * @brief Record completion of a context generation.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] generation Application context generation.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_taskEnd(uint32_t contextId, uint32_t generation);
#endif
#if KYMO_EFFECTIVE_TRACE_SCHEDULER || defined(DOXYGEN)

    /**
     * @brief Record core state: zero unknown, one running task, two idle; recovery is zero or one.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] state Core-state code described above.
     * @param[in] isRecovery One for recovery, otherwise zero.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_coreState(uint32_t contextId, uint8_t state, uint8_t isRecovery);
#endif
#if KYMO_EFFECTIVE_TRACE_FLOW || defined(DOXYGEN)

    /**
     * @brief Record the send side of a correlated operation.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] symbolId Nonzero generated symbol identifier.
     * @param[in] correlationId Identifier matching the send and receive operations.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_flowSend(uint32_t contextId, uint16_t symbolId, uint32_t correlationId);
#endif
#if KYMO_EFFECTIVE_TRACE_FLOW || defined(DOXYGEN)

    /**
     * @brief Record the receive side of a correlated operation.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] symbolId Nonzero generated symbol identifier.
     * @param[in] correlationId Identifier matching the send and receive operations.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    void KymoCoreTrace_flowReceive(uint32_t contextId, uint16_t symbolId, uint32_t correlationId);
#endif

    /**
     * @brief Reset an inactive, fully unwound context to a new generation.
     * @param[in] contextId Application context identifier; use a nonzero ID for scoped tracing.
     * @param[in] generation Application context generation.
     * @return OK, BAD_CONFIG before initialization, or REJECTED while the context is active.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    KymoStatus KymoCoreTrace_contextReset(uint32_t contextId, uint32_t generation);

    /**
     * @brief Read the dropped-event count for one core.
     * @param[in] coreId Core index below KYMO_TRACE_CORE_COUNT.
     * @return Current count; zero before initialization or for an invalid core.
     * @par Usage Call from an application context covered by the configured synchronization.
     */
    uint32_t KymoCoreTrace_dropCount(uint8_t coreId);

    /**
     * @brief Test whether startup metadata and dictionary have reached TX.
     * @return Nonzero when running and ready; zero before Init or after a fault.
     * @par Usage Optional diagnostic; hooks can record before metadata completes.
     */
    uint8_t KymoCoreTrace_isReady(void);
#endif
#ifdef __cplusplus
}
#if KYMO_EFFECTIVE_CORE || defined(DOXYGEN)

/**
 * @brief Automatic C++ entry/exit instrumentation for one lexical scope.
 * @details No allocation; destruction pairs the entry on every return path.
 * @par Usage Construct a local scope after successful Trace Init.
 */
class KymoCoreTraceScope
{
  public:

    /**
     * @brief Record scope entry.
     * @param[in] contextId Recording context identifier.
     * @param[in] symbolId Nonzero generated symbol identifier.
     * @par Usage Declare one local object per traced function/scope.
     */
    KymoCoreTraceScope(uint32_t contextId, uint16_t symbolId)
        : m_contextId(contextId), m_symbolId(symbolId)
    {
        KymoCoreTrace_enter(m_contextId, m_symbolId);
    }

    /**
     * @brief Record the matching scope exit.
     * @par Usage Invoked automatically when the object leaves scope.
     */
    ~KymoCoreTraceScope()
    {
        KymoCoreTrace_exit(m_contextId, m_symbolId);
    }

    KymoCoreTraceScope(const KymoCoreTraceScope &) = delete;
    KymoCoreTraceScope &operator=(const KymoCoreTraceScope &) = delete;

  private:

    /** @brief Recording context identifier. */
    uint32_t m_contextId;

    /** @brief Paired symbol identifier. */
    uint16_t m_symbolId;
};
#endif
#endif
#endif
#endif
