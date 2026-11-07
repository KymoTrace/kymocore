/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file KymoCore.h
 * @brief The one header applications include.
 *
 * @details KymoCore is organized by task; this header pulls in every module that
 * the build configuration enables:
 *
 * | Folder / header                  | Module                         | Enabled by            |
 * |----------------------------------|--------------------------------|-----------------------|
 * | kymo_config.h                    | Version, switches, sizes       | always                |
 * | kymo_types.h                     | KymoStatus, shared callbacks   | always                |
 * | telemetry/kymo_protocol.h        | DATA frame codec               | always                |
 * | telemetry/kymo_runtime.h         | Kymo_Init / Kymo_Main          | KYMO_ENABLE_RUNTIME=1 |
 * | telemetry/kymo_sender.h          | KymoSender (C++ push API)      | KYMO_ENABLE_CPP=1     |
 * | control/kymo_param.h             | KymoParam (host back channel)  | KYMO_ENABLE_PARAMETERS=1 |
 * | control/kymo_watch.h             | KymoWatch (live variables)     | KYMO_ENABLE_WATCH=1   |
 * | trace/kymo_instrumentation.h     | KYMO_TRACE_* macros            | always (compile-out)  |
 * | trace/kymo_trace.h               | KymoTraceRecorder              | KYMO_ENABLE_TRACE=1   |
 * | transport/kymo_mux.h             | KymoMux (trace + DATA link)    | KYMO_ENABLE_TRACE=1   |
 *
 * The library is platform-independent: no SDK, vendor or framework headers are
 * included anywhere. Hardware access always lives in application callbacks.
 * The header adds no declarations of its own and is valid in C and C++.
 * With KYMO_ENABLED=0 every module compiles out.
 * @par Usage
 * @code{.cpp}
 * #include <KymoCore.h>
 * static KymoContext context;
 * // Fill a KymoConfig by name, then: Kymo_Init(&context, &config); Kymo_Main(&context);
 * @endcode
 */
#ifndef KYMO_CORE_H
#define KYMO_CORE_H

#include "kymo_config.h"
#include "kymo_types.h"
#include "telemetry/kymo_protocol.h"
#include "telemetry/kymo_runtime.h"
#include "control/kymo_param.h"
#include "control/kymo_watch.h"
#include "trace/kymo_instrumentation.h"

#if KYMO_EFFECTIVE_CPP && defined(__cplusplus)
#include "telemetry/kymo_sender.h"
#endif

#if KYMO_EFFECTIVE_TRACE
#include "transport/kymo_mux.h"
#endif

#endif /* KYMO_CORE_H */
