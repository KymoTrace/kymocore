/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */

/**
 * @file kymo_instrumentation.h
 * @brief Compile-time facade for optional Kymo trace instrumentation.
 * @details Disabled macros expand to a valid no-op statement and discard every
 * argument token before C/C++ semantic analysis. Active macros delegate to the
 * public trace API. C++ scope instrumentation is independent of KYMO_ENABLE_CPP.
 * @par Usage
 * Include this header at an instrumentation call site and configure KymoCore once.
 */
#ifndef KYMO_INSTRUMENTATION_H
#define KYMO_INSTRUMENTATION_H

#include "../kymo_config.h"

#if KYMO_EFFECTIVE_TRACE
#include "kymo_trace.h"

/** @brief Record entry into the given symbol/context. @par Usage KYMO_TRACE_ENTER(recorder, context, symbol); */
#define KYMO_TRACE_ENTER(recorder_, context_, symbol_) \
    ((void)KymoTrace_Enter((recorder_), (context_), (symbol_)))

/** @brief Record exit from the given symbol/context. @par Usage KYMO_TRACE_EXIT(recorder, context, symbol); */
#define KYMO_TRACE_EXIT(recorder_, context_, symbol_) \
    ((void)KymoTrace_Exit((recorder_), (context_), (symbol_)))

/** @brief Record a marker value. @par Usage KYMO_TRACE_MARK(recorder, context, marker, value); */
#define KYMO_TRACE_MARK(recorder_, context_, marker_, value_) \
    ((void)KymoTrace_Mark((recorder_), (context_), (marker_), (value_)))
#if KYMO_EFFECTIVE_TRACE_ISR

/** @brief Record ISR entry. @par Usage KYMO_TRACE_ISR_ENTER(recorder, irq_context); */
#define KYMO_TRACE_ISR_ENTER(recorder_, irq_context_) \
    ((void)KymoTrace_IsrEnter((recorder_), (irq_context_)))

/** @brief Record ISR exit. @par Usage KYMO_TRACE_ISR_EXIT(recorder, irq_context); */
#define KYMO_TRACE_ISR_EXIT(recorder_, irq_context_) \
    ((void)KymoTrace_IsrExit((recorder_), (irq_context_)))
#else

/** @brief Compile out ISR entry when ISR tracing is disabled. @par Usage Use as a statement. */
#define KYMO_TRACE_ISR_ENTER(...) ((void)0)

/** @brief Compile out ISR exit when ISR tracing is disabled. @par Usage Use as a statement. */
#define KYMO_TRACE_ISR_EXIT(...) ((void)0)
#endif

#if KYMO_EFFECTIVE_TRACE_SCHEDULER

/** @brief Record a scheduler switch. @par Usage KYMO_TRACE_SCHEDULER(recorder, context, task); */
#define KYMO_TRACE_SCHEDULER(recorder_, context_, task_) \
    ((void)KymoTrace_Scheduler((recorder_), (context_), (task_)))
#else

/** @brief Compile out scheduler recording when disabled. @par Usage Use as a statement. */
#define KYMO_TRACE_SCHEDULER(...) ((void)0)
#endif

#if KYMO_EFFECTIVE_TRACE_FLOW

/** @brief Record correlation send. @par Usage KYMO_TRACE_FLOW_SEND(recorder, context, symbol, id); */
#define KYMO_TRACE_FLOW_SEND(recorder_, context_, symbol_, correlation_) \
    ((void)KymoTrace_FlowSend((recorder_), (context_), (symbol_), (correlation_)))

/** @brief Record correlation receive. @par Usage KYMO_TRACE_FLOW_RECEIVE(recorder, context, symbol, id); */
#define KYMO_TRACE_FLOW_RECEIVE(recorder_, context_, symbol_, correlation_) \
    ((void)KymoTrace_FlowReceive((recorder_), (context_), (symbol_), (correlation_)))

/** @brief Legacy alias for flow send. @par Usage Prefer explicit send/receive macros. */
#define KYMO_TRACE_FLOW(recorder_, context_, symbol_, correlation_) \
    KYMO_TRACE_FLOW_SEND((recorder_), (context_), (symbol_), (correlation_))
#else

/** @brief Compile out flow send and discard arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_FLOW_SEND(...) ((void)0)

/** @brief Compile out flow receive and discard arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_FLOW_RECEIVE(...) ((void)0)

/** @brief Compile out flow alias and discard arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_FLOW(...) ((void)0)
#endif

#ifdef __cplusplus
#define KYMO_TRACE_DETAIL_JOIN_I(left_, right_) left_##right_
#define KYMO_TRACE_DETAIL_JOIN(left_, right_) KYMO_TRACE_DETAIL_JOIN_I(left_, right_)
#ifndef __COUNTER__
#define __COUNTER__ __LINE__
#endif

#define KYMO_TRACE_SCOPE_IMPL(recorder_, context_, symbol_, unique_)                         \
    KymoTraceScope KYMO_TRACE_DETAIL_JOIN(kymoTraceScope_, unique_)((recorder_), (context_), \
                                                                    (symbol_))

/** @brief Create a C++ RAII trace scope. @par Usage Place in the block to measure. */
#define KYMO_TRACE_SCOPE(recorder_, context_, symbol_) \
    KYMO_TRACE_SCOPE_IMPL((recorder_), (context_), (symbol_), __COUNTER__)
#else

/** @brief Compile out C++ scope syntax for a C caller. @par Usage Use as a statement. */
#define KYMO_TRACE_SCOPE(...) ((void)0)
#endif
#else

/** @brief Compile out trace entry and discard all arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_ENTER(...) ((void)0)

/** @brief Compile out trace exit and discard all arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_EXIT(...) ((void)0)

/** @brief Compile out trace marker and discard all arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_MARK(...) ((void)0)

/** @brief Compile out ISR entry and discard all arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_ISR_ENTER(...) ((void)0)

/** @brief Compile out ISR exit and discard all arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_ISR_EXIT(...) ((void)0)

/** @brief Compile out scheduler recording and discard all arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_SCHEDULER(...) ((void)0)

/** @brief Compile out flow send. @par Usage Use as a statement. */
#define KYMO_TRACE_FLOW_SEND(...) ((void)0)

/** @brief Compile out flow receive. @par Usage Use as a statement. */
#define KYMO_TRACE_FLOW_RECEIVE(...) ((void)0)

/** @brief Compile out flow alias. @par Usage Use as a statement. */
#define KYMO_TRACE_FLOW(...) ((void)0)

/** @brief Compile out C++ scope and discard arguments. @par Usage Use as a statement. */
#define KYMO_TRACE_SCOPE(...) ((void)0)
#endif

#endif /* KYMO_INSTRUMENTATION_H */
