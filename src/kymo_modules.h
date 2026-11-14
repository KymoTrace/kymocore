/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_modules.h
 * @brief Explicit-instance entry for direct module consumers.
 *
 * @details KymoCore is organized by task; this header pulls in every module that
 * the build configuration enables:
 *
 * | Folder / header                  | Module                         | Enabled by            |
 * |----------------------------------|--------------------------------|-----------------------|
 * | kymo_config.h                    | Profile, switches, sizes       | always                |
 * | kymo_version.h                   | Library version                | always                |
 * | kymo_config_check.h              | Validation, KYMO_EFFECTIVE_*   | always (internal)     |
 * | kymo_types.h                     | KymoStatus, shared callbacks   | always                |
 * | communication/kymo_protocol.h        | DATA frame codec               | always                |
 * | features/plot/kymo_runtime.h         | KymoRuntime_init / KymoRuntime_main          | KYMO_ENABLE_RUNTIME=1 |
 * | features/plot/kymo_sender.h          | KymoSender (C++ push API)      | KYMO_ENABLE_CPP=1     |
 * | features/parameters/kymo_param.h             | KymoParam (host back channel)  | KYMO_ENABLE_PARAMETERS=1 |
 * | features/watch/kymo_watch.h             | KymoWatch (live variables)     | KYMO_ENABLE_WATCH=1   |
 * | features/trace/kymo_trace_instrumentation.h     | KYMO_TRACE_* macros            | always (compile-out)  |
 * | features/trace/kymo_trace.h               | KymoTraceRecorder              | KYMO_ENABLE_TRACE=1   |
 * | core/kymo_core_endpoint.h         | Modular Init/Main coordinator | KYMO_ENABLE_ENDPOINT=1 |
 * | communication/kymo_tx.h              | KymoTx (shared frame sender)   | KYMO_ENABLE_TX=1      |
 * | communication/kymo_mux.h             | KymoMux (trace + DATA link)    | KYMO_ENABLE_TRACE=1   |
 *
 * The library is platform-independent: no SDK, vendor or framework headers are
 * included anywhere. Hardware access always lives in application callbacks.
 * The header adds no declarations of its own and is valid in C and C++.
 * With KYMO_ENABLED=0 every module compiles out.
 * @par Usage
 * @code{.cpp}
 * #include <kymo_core.h>
 * static KymoContext context;
 * // Fill a KymoConfig by name, then: KymoRuntime_init(&context, &config); KymoRuntime_main(&context);
 * @endcode
 */
#ifndef KYMO_MODULES_H
#define KYMO_MODULES_H

#include "kymo_config.h"
#include "kymo_types.h"
#include "core/kymo_core_endpoint.h"
#include "communication/kymo_tx.h"
#include "communication/kymo_protocol.h"
#include "features/plot/kymo_runtime.h"
#include "features/parameters/kymo_param.h"
#include "features/watch/kymo_watch.h"
#include "features/memory/kymo_memory.h"
#include "features/trace/kymo_trace_instrumentation.h"

#if KYMO_EFFECTIVE_CPP && defined(__cplusplus)
#include "features/plot/kymo_sender.h"
#endif

#if KYMO_EFFECTIVE_TRACE
#include "communication/kymo_mux.h"
#endif

#endif /* KYMO_MODULES_H */
