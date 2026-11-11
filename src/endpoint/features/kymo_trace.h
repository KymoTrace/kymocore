/* SPDX-License-Identifier: MIT */
/** @file kymo_trace.h
 * @brief Trace feature initialization for the shared Endpoint.
 * @details No unrelated feature interfaces. All calls are serialized; tables and
 * callback contexts must outlive the feature. State is caller-owned and must be
 * zero-initialized before Init. The Endpoint owns cyclic processing afterward.
 * @par Usage Initialize the Endpoint, then this feature, then Endpoint Main.
 */
#ifndef KYMO_ENDPOINT_FEATURE_TRACE_H
#define KYMO_ENDPOINT_FEATURE_TRACE_H
#include "../detail/kymo_feature.h"
#include "../../transport/kymo_mux.h"
#if (KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE) || defined(DOXYGEN)
#ifdef __cplusplus
extern "C" {
#endif
/** @brief Trace-only settings; no physical writer or other feature configs.
 * @details Copied by Init; referenced arrays and contexts remain caller-owned.
 * @par Usage Zero-initialize and assign named fields. */
typedef struct {
    KymoTraceConfig recorder; /**< Clock, session and synchronization callbacks. */
    const uint8_t *metadata; /**< Persistent canonical session metadata JSON, excluding CRC. */
    size_t metadata_length; /**< Metadata bytes, excluding string terminator. */
    const uint8_t *dictionary; /**< Persistent canonical dictionary JSON, excluding CRC. */
    size_t dictionary_length; /**< Dictionary bytes, excluding string terminator. */
    uint32_t generation; /**< Record generation; nonzero. */
    uint32_t health_period_ticks; /**< Optional health interval; zero disables it. */
} KymoEndpointTraceConfig;
/** @brief Persistent Trace context; exposed solely for static allocation.
 * @details Internal fields are read-only; never move/reinitialize a bound feature.
 * @par Usage Declare static storage or zero-initialize before the first Init. */
typedef struct {
    KymoEndpointFeature binding; /**< Internal ownership and dispatch. */
    KymoTraceRecorder recorder; /**< Owned event recorder. */
    KymoMux mux; /**< Owned trace/admin mux. */
    KymoEndpointTraceConfig config; /**< Copied trace configuration. */
    uint8_t metadata_stage; /**< Internal metadata progress. */
    uint8_t end_queued; /**< Session end submitted exactly once. */
} KymoEndpointTrace;
/** @brief Initialize and register Trace completely, without physical I/O.
 * @details One instance per feature type per Endpoint. Failed setup latches a
 * configuration error on a configuring Endpoint; late setup leaves a running
 * Endpoint unchanged. Shared channel reservations reject cross-feature overlap.
 * @param[in,out] endpoint Base in configuring phase.
 * @param[in,out] feature Zero-initialized feature storage.
 * @param[in] config Trace settings; NULL is invalid.
 * @return OK, BAD_CONFIG for invalid resources, REJECTED for duplicate/late
 * initialization, or the existing terminal error.
 * @par Usage Check success before starting the common Main loop. */
KymoStatus KymoEndpointTrace_Init(KymoEndpoint *endpoint, KymoEndpointTrace *feature,
    const KymoEndpointTraceConfig *config);
/** @brief Obtain the recorder only during configuration/running phases.
 * @details Do not retain the returned pointer across stop/reinitialization.
 * Calls read Endpoint phase: externally serialize hooks with Main/stop; recorder
 * locking alone does not protect Endpoint state. Quiesce hooks before stop.
 * Recorder locks must match execution contexts. No implicit ISR safety.
 * @param[in,out] feature Trace context; NULL is allowed.
 * @return Recorder or NULL before successful Init/after stop/fault.
 * @par Usage Prefer the feature instrumentation macros below. */
KymoTraceRecorder *KymoEndpointTrace_Recorder(KymoEndpointTrace *feature);
/** @brief Check that session metadata and dictionary have been handed to TX.
 * @param[in] feature Trace context; NULL is allowed.
 * @return Nonzero when initialized, running and metadata is queued completely.
 * @par Usage Optional diagnostic; recording may begin before metadata completes. */
uint8_t KymoEndpointTrace_IsReady(const KymoEndpointTrace *feature);
#ifdef __cplusplus
}
#endif
#endif
#include "../../trace/kymo_instrumentation.h"
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE
/** @brief Enter a named scope; evaluates each argument once.
 * @par Usage Use feature pointer, context and generated symbol identifiers. */
#define KYMO_ENDPOINT_TRACE_ENTER(feature_, context_, symbol_) \
    KYMO_TRACE_ENTER(KymoEndpointTrace_Recorder((feature_)), (context_), (symbol_))
#else
/** @brief Disabled enter a named scope; discards all arguments.
 * @par Usage Valid no-op statement, including if/else bodies. */
#define KYMO_ENDPOINT_TRACE_ENTER(...) ((void)0)
#endif
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE
/** @brief Exit a named scope; evaluates each argument once.
 * @par Usage Use feature pointer, context and generated symbol identifiers. */
#define KYMO_ENDPOINT_TRACE_EXIT(feature_, context_, symbol_) \
    KYMO_TRACE_EXIT(KymoEndpointTrace_Recorder((feature_)), (context_), (symbol_))
#else
/** @brief Disabled exit a named scope; discards all arguments.
 * @par Usage Valid no-op statement, including if/else bodies. */
#define KYMO_ENDPOINT_TRACE_EXIT(...) ((void)0)
#endif
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE
/** @brief Record a named marker; evaluates each argument once.
 * @par Usage Use feature pointer, context and generated symbol identifiers. */
#define KYMO_ENDPOINT_TRACE_MARK(feature_, context_, symbol_, value_) \
    KYMO_TRACE_MARK(KymoEndpointTrace_Recorder((feature_)), (context_), (symbol_), (value_))
#else
/** @brief Disabled record a named marker; discards all arguments.
 * @par Usage Valid no-op statement, including if/else bodies. */
#define KYMO_ENDPOINT_TRACE_MARK(...) ((void)0)
#endif
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE && defined(__cplusplus)
/** @brief Record a C++ scope with automatic exit; evaluates each argument once.
 * @par Usage Use feature pointer, context and generated symbol identifiers. */
#define KYMO_ENDPOINT_TRACE_SCOPE(feature_, context_, symbol_) \
    KYMO_TRACE_SCOPE(KymoEndpointTrace_Recorder((feature_)), (context_), (symbol_))
#else
/** @brief Disabled record a c++ scope with automatic exit; discards all arguments.
 * @par Usage Valid no-op statement, including if/else bodies. */
#define KYMO_ENDPOINT_TRACE_SCOPE(...) ((void)0)
#endif
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE_ISR
/** @brief Record ISR entry; evaluates each argument once.
 * @par Usage Use feature pointer, context and generated symbol identifiers. */
#define KYMO_ENDPOINT_TRACE_ISR_ENTER(feature_, context_) \
    KYMO_TRACE_ISR_ENTER(KymoEndpointTrace_Recorder((feature_)), (context_))
#else
/** @brief Disabled record isr entry; discards all arguments.
 * @par Usage Valid no-op statement, including if/else bodies. */
#define KYMO_ENDPOINT_TRACE_ISR_ENTER(...) ((void)0)
#endif
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE_ISR
/** @brief Record ISR exit; evaluates each argument once.
 * @par Usage Use feature pointer, context and generated symbol identifiers. */
#define KYMO_ENDPOINT_TRACE_ISR_EXIT(feature_, context_) \
    KYMO_TRACE_ISR_EXIT(KymoEndpointTrace_Recorder((feature_)), (context_))
#else
/** @brief Disabled record isr exit; discards all arguments.
 * @par Usage Valid no-op statement, including if/else bodies. */
#define KYMO_ENDPOINT_TRACE_ISR_EXIT(...) ((void)0)
#endif
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE_SCHEDULER
/** @brief Record a task switch; evaluates each argument once.
 * @par Usage Use feature pointer, context and generated symbol identifiers. */
#define KYMO_ENDPOINT_TRACE_SCHEDULER(feature_, context_, task_) \
    KYMO_TRACE_SCHEDULER(KymoEndpointTrace_Recorder((feature_)), (context_), (task_))
#else
/** @brief Disabled record a task switch; discards all arguments.
 * @par Usage Valid no-op statement, including if/else bodies. */
#define KYMO_ENDPOINT_TRACE_SCHEDULER(...) ((void)0)
#endif
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE_FLOW
/** @brief Record a correlation send; evaluates each argument once.
 * @par Usage Use feature pointer, context and generated symbol identifiers. */
#define KYMO_ENDPOINT_TRACE_FLOW_SEND(feature_, context_, symbol_, id_) \
    KYMO_TRACE_FLOW_SEND(KymoEndpointTrace_Recorder((feature_)), (context_), (symbol_), (id_))
#else
/** @brief Disabled record a correlation send; discards all arguments.
 * @par Usage Valid no-op statement, including if/else bodies. */
#define KYMO_ENDPOINT_TRACE_FLOW_SEND(...) ((void)0)
#endif
#if KYMO_EFFECTIVE_ENDPOINT && KYMO_EFFECTIVE_TRACE_FLOW
/** @brief Record a correlation receive; evaluates each argument once.
 * @par Usage Use feature pointer, context and generated symbol identifiers. */
#define KYMO_ENDPOINT_TRACE_FLOW_RECEIVE(feature_, context_, symbol_, id_) \
    KYMO_TRACE_FLOW_RECEIVE(KymoEndpointTrace_Recorder((feature_)), (context_), (symbol_), (id_))
#else
/** @brief Disabled record a correlation receive; discards all arguments.
 * @par Usage Valid no-op statement, including if/else bodies. */
#define KYMO_ENDPOINT_TRACE_FLOW_RECEIVE(...) ((void)0)
#endif
#endif /* KYMO_ENDPOINT_FEATURE_TRACE_H */
