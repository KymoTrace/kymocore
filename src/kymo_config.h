/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */

/**
 * @file kymo_config.h
 * @brief The one place for every KymoCore build setting.
 * @details Resolves version, profile, feature switches and resource sizes, validates
 * them and derives the KYMO_EFFECTIVE_* macros that the library and applications
 * test. All settings are preprocessor-only and create no firmware resources.
 *
 * Precedence: compiler definitions (-D...) override the optional project header
 * named by KYMO_USER_CONFIG_HEADER, which overrides the profile, which overrides
 * the library defaults below.
 *
 * | Group                 | Setting                     | Default | Effect                                  |
 * |-----------------------|-----------------------------|---------|-----------------------------------------|
 * | Master                | KYMO_ENABLED                | 1       | 0 compiles the whole library out        |
 * | Profile               | KYMO_PROFILE                | MINIMAL | Preset defaults for the switches below  |
 * | 1. Telemetry API      | KYMO_ENABLE_RUNTIME         | 1       | Cyclic Kymo_Init/Kymo_Main (recommended)|
 * |                       | KYMO_ENABLE_CPP             | 0       | Optional C++ push sender (KymoSender)   |
 * | 2. Telemetry payload  | KYMO_ENABLE_X               | 0       | X coordinate per sample                 |
 * |                       | KYMO_ENABLE_Z               | 0       | Z coordinate per sample                 |
 * |                       | KYMO_ENABLE_TIMESTAMP       | 0       | Timestamp per sample                    |
 * |                       | KYMO_ENABLE_CRC             | 0       | CRC-8 on outgoing frames                |
 * |                       | KYMO_ENABLE_DECODER         | 0       | Frame decoder on the MCU                |
 * | 2b. Back channel      | KYMO_ENABLE_PARAMETERS      | 0       | Host writes to registered parameters    |
 * |                       | KYMO_ENABLE_WATCH           | 0       | Host samples variables by address (ELF) |
 * | 3. Execution tracing  | KYMO_ENABLE_TRACE           | 0       | Trace recorder, instrumentation, mux    |
 * |                       | KYMO_ENABLE_TRACE_ISR       | 0       | ISR enter/exit (needs TRACE)            |
 * |                       | KYMO_ENABLE_TRACE_SCHEDULER | 0       | Task switches (needs TRACE_ISR)         |
 * |                       | KYMO_ENABLE_TRACE_FLOW      | 0       | Send/receive correlation (needs TRACE)  |
 * |                       | KYMO_ENABLE_TRACE_STACK_POINTER | 0   | Stack pointer in ENTER/EXIT (TRACE)     |
 * | 4. Resource sizes     | KYMO_MAX_CHANNELS           | 16      | Telemetry channels per context (1..256) |
 * |                       | KYMO_TRACE_*                | see below | Trace ring, contexts, depths          |
 * |                       | KYMO_MUX_ADMIN_RECORD_MAX   | 512     | Largest queued admin record in bytes    |
 * |                       | KYMO_MAX_PARAMETERS         | 8       | Parameters per receiver (1..256)        |
 * |                       | KYMO_MAX_WATCHES            | 8       | Live-variable slots (1..64)             |
 * |                       | KYMO_WATCH_MIN_PERIOD_MS    | 5       | Shortest sampling period (1..60000)     |
 *
 * Profiles only change defaults; every switch can still be set individually:
 * - KYMO_PROFILE_MINIMAL (0): scalar values, no timestamp, no tracing.
 * - KYMO_PROFILE_XYZ (1): X, Z and timestamps enabled.
 * - KYMO_PROFILE_TRACE (2): XYZ plus trace recorder with ISR, scheduler and flow events.
 *
 * Every boolean switch accepts only literal 0 or 1.
 * @par Usage
 * Pass settings with -D, or set KYMO_USER_CONFIG_HEADER to a quoted header path.
 * @code
 * -DKYMO_PROFILE=KYMO_PROFILE_XYZ -DKYMO_MAX_CHANNELS=4
 * @endcode
 */
#ifndef KYMO_CONFIG_H
#define KYMO_CONFIG_H

#if defined(__cplusplus) && (__cplusplus < 201103L)
#error "KymoCore requires C++11 or newer"
#endif

/* ---- Version ------------------------------------------------------------ */

/** @brief Library major version; incremented for incompatible API changes. */
#define KYMO_VERSION_MAJOR 7
/** @brief Library minor version; incremented for compatible additions. */
#define KYMO_VERSION_MINOR 2
/** @brief Library patch version; incremented for compatible fixes. */
#define KYMO_VERSION_PATCH 1
/** @brief Library version as a string literal. */
#define KYMO_VERSION_STRING "7.2.1"

/** @brief Version of the compile-time configuration contract consumed by tools. */
#define KYMO_CONFIG_API_VERSION 2

#if defined(KYMO_USER_CONFIG_HEADER)
#include KYMO_USER_CONFIG_HEADER
#endif

/* ---- Master switch and profile ----------------------------------------- */

/** @brief Global KymoCore gate; defaults to enabled. Define 0 for compile-out. */
#ifndef KYMO_ENABLED
#define KYMO_ENABLED 1
#endif

/** @brief Profile value: scalar telemetry only (library default). */
#define KYMO_PROFILE_MINIMAL 0
/** @brief Profile value: X, Z and timestamp enabled. */
#define KYMO_PROFILE_XYZ 1
/** @brief Profile value: XYZ plus complete execution tracing. */
#define KYMO_PROFILE_TRACE 2

/** @brief Selected preset; changes only the defaults of the switches below. */
#ifndef KYMO_PROFILE
#define KYMO_PROFILE KYMO_PROFILE_MINIMAL
#endif

/** @brief Internal token-paste step for checking literal settings. */
#define KYMO_DETAIL_PASTE_I(prefix_, value_) prefix_##value_
/** @brief Internal expansion wrapper so macro-valued settings are resolved first. */
#define KYMO_DETAIL_PASTE(prefix_, value_) KYMO_DETAIL_PASTE_I(prefix_, value_)
/** @brief Internal marker: literal 0 is a valid boolean or profile. */
#define KYMO_DETAIL_VALID_0 1
/** @brief Internal marker: literal 1 is a valid boolean or profile. */
#define KYMO_DETAIL_VALID_1 1
/** @brief Internal marker: literal 2 is a valid profile. */
#define KYMO_DETAIL_PROFILE_2 1

#if !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_PROFILE) && \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_PROFILE_, KYMO_PROFILE)
#error "KYMO_PROFILE must be KYMO_PROFILE_MINIMAL, KYMO_PROFILE_XYZ or KYMO_PROFILE_TRACE"
#endif

/* ---- 1. Telemetry API --------------------------------------------------- */

/** @brief Enable cooperative Init/Main scheduling; defaults to enabled. */
#ifndef KYMO_ENABLE_RUNTIME
#define KYMO_ENABLE_RUNTIME 1
#endif

/** @brief Compile the optional C++ push sender; defaults to disabled. */
#ifndef KYMO_ENABLE_CPP
#define KYMO_ENABLE_CPP 0
#endif

/* ---- 2. Telemetry payload ----------------------------------------------- */

/** @brief Include X coordinates in the wire payload; on in the XYZ and TRACE profiles. */
#ifndef KYMO_ENABLE_X
#if KYMO_PROFILE >= KYMO_PROFILE_XYZ
#define KYMO_ENABLE_X 1
#else
#define KYMO_ENABLE_X 0
#endif
#endif

/** @brief Include Z coordinates in the wire payload; on in the XYZ and TRACE profiles. */
#ifndef KYMO_ENABLE_Z
#if KYMO_PROFILE >= KYMO_PROFILE_XYZ
#define KYMO_ENABLE_Z 1
#else
#define KYMO_ENABLE_Z 0
#endif
#endif

/** @brief Include timestamps in DATA frames; on in the XYZ and TRACE profiles. */
#ifndef KYMO_ENABLE_TIMESTAMP
#if KYMO_PROFILE >= KYMO_PROFILE_XYZ
#define KYMO_ENABLE_TIMESTAMP 1
#else
#define KYMO_ENABLE_TIMESTAMP 0
#endif
#endif

/** @brief Protect outgoing DATA frames with CRC; defaults to disabled. */
#ifndef KYMO_ENABLE_CRC
#define KYMO_ENABLE_CRC 0
#endif

/** @brief Compile complete-frame MCU decoding; defaults to disabled. */
#ifndef KYMO_ENABLE_DECODER
#define KYMO_ENABLE_DECODER 0
#endif

/* ---- 2b. Back channel --------------------------------------------------- */

/** @brief Compile the parameter receiver for host writes; requires the decoder; defaults to disabled. */
#ifndef KYMO_ENABLE_PARAMETERS
#define KYMO_ENABLE_PARAMETERS 0
#endif

/** @brief Compile live variables: the host samples memory by address; requires the decoder; defaults to disabled. */
#ifndef KYMO_ENABLE_WATCH
#define KYMO_ENABLE_WATCH 0
#endif

/* ---- 3. Execution tracing ----------------------------------------------- */

/** @brief Compile the trace recorder, instrumentation and mux; on in the TRACE profile. */
#ifndef KYMO_ENABLE_TRACE
#if KYMO_PROFILE == KYMO_PROFILE_TRACE
#define KYMO_ENABLE_TRACE 1
#else
#define KYMO_ENABLE_TRACE 0
#endif
#endif

/** @brief Compile ISR tracing; requires trace; on in the TRACE profile. */
#ifndef KYMO_ENABLE_TRACE_ISR
#if KYMO_PROFILE == KYMO_PROFILE_TRACE
#define KYMO_ENABLE_TRACE_ISR 1
#else
#define KYMO_ENABLE_TRACE_ISR 0
#endif
#endif

/** @brief Compile scheduler tracing; requires ISR tracing; on in the TRACE profile. */
#ifndef KYMO_ENABLE_TRACE_SCHEDULER
#if KYMO_PROFILE == KYMO_PROFILE_TRACE
#define KYMO_ENABLE_TRACE_SCHEDULER 1
#else
#define KYMO_ENABLE_TRACE_SCHEDULER 0
#endif
#endif

/** @brief Compile flow correlation tracing; requires trace; on in the TRACE profile. */
#ifndef KYMO_ENABLE_TRACE_FLOW
#if KYMO_PROFILE == KYMO_PROFILE_TRACE
#define KYMO_ENABLE_TRACE_FLOW 1
#else
#define KYMO_ENABLE_TRACE_FLOW 0
#endif
#endif

/** @brief Record the stack pointer in ENTER/EXIT events; requires trace; defaults to disabled.
 * @details The value goes into the otherwise unused arg field and sets
 * KYMO_TRACE_FLAG_STACK_SAMPLE, so events keep their 32 bytes. The host derives the
 * stack depth per context from it. Reading costs one compiler builtin per event.
 * @par Usage
 * -DKYMO_ENABLE_TRACE_STACK_POINTER=1; on compilers without __builtin_frame_address
 * also define KYMO_TRACE_READ_STACK_POINTER() to return the current stack address.
 */
#ifndef KYMO_ENABLE_TRACE_STACK_POINTER
#define KYMO_ENABLE_TRACE_STACK_POINTER 0
#endif

/* ---- 4. Resource sizes -------------------------------------------------- */

/** @brief Telemetry channels a single KymoContext can schedule (1..256).
 * @details Each channel costs four bytes of scheduling state inside KymoContext.
 */
#ifndef KYMO_MAX_CHANNELS
#define KYMO_MAX_CHANNELS 16u
#endif

/** @brief Trace event ring capacity per core (power of two, 16..4096). */
#ifndef KYMO_TRACE_EVENTS_PER_CORE
#define KYMO_TRACE_EVENTS_PER_CORE 64u
#endif

/** @brief Number of recorder cores supported (1 or 2). */
#ifndef KYMO_TRACE_CORE_COUNT
#define KYMO_TRACE_CORE_COUNT 1u
#endif

/** @brief Maximum tracked trace contexts such as tasks or ISRs (1..256). */
#ifndef KYMO_TRACE_MAX_CONTEXTS
#define KYMO_TRACE_MAX_CONTEXTS 16u
#endif

/** @brief Maximum stored scope symbols per trace context (1..255). */
#ifndef KYMO_TRACE_MAX_DEPTH
#define KYMO_TRACE_MAX_DEPTH 32u
#endif

/** @brief Maximum nested ISR activations when ISR tracing is effective (1..16). */
#ifndef KYMO_TRACE_MAX_ISR_DEPTH
#define KYMO_TRACE_MAX_ISR_DEPTH 8u
#endif

/** @brief Largest administrative record the mux can queue, in bytes (64..8192). */
#ifndef KYMO_MUX_ADMIN_RECORD_MAX
#define KYMO_MUX_ADMIN_RECORD_MAX 512u
#endif

/** @brief Parameters one KymoParamReceiver can hold (1..256).
 * @details Each parameter costs one bit of pending-readback state inside the receiver;
 * the parameter table itself is owned by the application.
 */
#ifndef KYMO_MAX_PARAMETERS
#define KYMO_MAX_PARAMETERS 8u
#endif

/** @brief Live-variable slots one KymoWatch can sample (1..64).
 * @details Each slot costs 16 bytes inside KymoWatch.
 */
#ifndef KYMO_MAX_WATCHES
#define KYMO_MAX_WATCHES 8u
#endif

/** @brief Shortest live-variable sampling period in milliseconds (1..60000).
 * @details Reported to the host; shorter requests are rejected.
 */
#ifndef KYMO_WATCH_MIN_PERIOD_MS
#define KYMO_WATCH_MIN_PERIOD_MS 5u
#endif

/* ---- Validation --------------------------------------------------------- */

#if !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLED) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_RUNTIME) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_CPP) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_X) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_Z) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TIMESTAMP) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_CRC) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_DECODER) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_PARAMETERS) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_WATCH) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE_ISR) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE_SCHEDULER) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE_FLOW) ||     !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE_STACK_POINTER)
#error "Kymo feature switches must be 0 or 1 (literal tokens only)"
#endif

#undef KYMO_DETAIL_PASTE
#undef KYMO_DETAIL_PASTE_I
#undef KYMO_DETAIL_VALID_0
#undef KYMO_DETAIL_VALID_1
#undef KYMO_DETAIL_PROFILE_2

#if KYMO_ENABLED && KYMO_ENABLE_TRACE_SCHEDULER && !KYMO_ENABLE_TRACE_ISR
#error "KYMO_ENABLE_TRACE_SCHEDULER requires KYMO_ENABLE_TRACE_ISR=1"
#endif

#if KYMO_ENABLED && KYMO_ENABLE_TRACE_ISR && !KYMO_ENABLE_TRACE
#error "KYMO_ENABLE_TRACE_ISR requires KYMO_ENABLE_TRACE=1"
#endif

#if KYMO_ENABLED && KYMO_ENABLE_TRACE_FLOW && !KYMO_ENABLE_TRACE
#error "KYMO_ENABLE_TRACE_FLOW requires KYMO_ENABLE_TRACE=1"
#endif

#if KYMO_ENABLED && KYMO_ENABLE_TRACE_STACK_POINTER && !KYMO_ENABLE_TRACE
#error "KYMO_ENABLE_TRACE_STACK_POINTER requires KYMO_ENABLE_TRACE=1"
#endif

#if KYMO_ENABLED && KYMO_ENABLE_PARAMETERS && !KYMO_ENABLE_DECODER
#error "KYMO_ENABLE_PARAMETERS requires KYMO_ENABLE_DECODER=1"
#endif

#if KYMO_ENABLED && KYMO_ENABLE_WATCH && !KYMO_ENABLE_DECODER
#error "KYMO_ENABLE_WATCH requires KYMO_ENABLE_DECODER=1"
#endif

#if (KYMO_MAX_WATCHES < 1u) || (KYMO_MAX_WATCHES > 64u)
#error "KYMO_MAX_WATCHES must be from 1 through 64"
#endif

#if (KYMO_WATCH_MIN_PERIOD_MS < 1u) || (KYMO_WATCH_MIN_PERIOD_MS > 60000u)
#error "KYMO_WATCH_MIN_PERIOD_MS must be from 1 through 60000"
#endif

#if (KYMO_MAX_PARAMETERS < 1u) || (KYMO_MAX_PARAMETERS > 256u)
#error "KYMO_MAX_PARAMETERS must be from 1 through 256"
#endif

#if (KYMO_MAX_CHANNELS < 1u) || (KYMO_MAX_CHANNELS > 256u)
#error "KYMO_MAX_CHANNELS must be from 1 through 256"
#endif

#if (KYMO_TRACE_EVENTS_PER_CORE < 16u) || (KYMO_TRACE_EVENTS_PER_CORE > 4096u) || \
    ((KYMO_TRACE_EVENTS_PER_CORE & (KYMO_TRACE_EVENTS_PER_CORE - 1u)) != 0u)
#error "KYMO_TRACE_EVENTS_PER_CORE must be a power of two from 16 through 4096"
#endif

#if (KYMO_TRACE_CORE_COUNT < 1u) || (KYMO_TRACE_CORE_COUNT > 2u)
#error "KYMO_TRACE_CORE_COUNT must be 1 or 2"
#endif

#if (KYMO_TRACE_MAX_CONTEXTS < 1u) || (KYMO_TRACE_MAX_CONTEXTS > 256u)
#error "KYMO_TRACE_MAX_CONTEXTS must be from 1 through 256"
#endif

#if (KYMO_TRACE_MAX_DEPTH < 1u) || (KYMO_TRACE_MAX_DEPTH > 255u)
#error "KYMO_TRACE_MAX_DEPTH must be from 1 through 255"
#endif

#if (KYMO_TRACE_MAX_ISR_DEPTH < 1u) || (KYMO_TRACE_MAX_ISR_DEPTH > 16u)
#error "KYMO_TRACE_MAX_ISR_DEPTH must be from 1 through 16"
#endif

#if (KYMO_MUX_ADMIN_RECORD_MAX < 64u) || (KYMO_MUX_ADMIN_RECORD_MAX > 8192u)
#error "KYMO_MUX_ADMIN_RECORD_MAX must be from 64 through 8192"
#endif

/* ---- Effective features ------------------------------------------------- */

/** @brief Effective cooperative runtime gate. */
#define KYMO_EFFECTIVE_RUNTIME (KYMO_ENABLED && KYMO_ENABLE_RUNTIME)

/** @brief Effective optional C++ sender gate. */
#define KYMO_EFFECTIVE_CPP (KYMO_ENABLED && KYMO_ENABLE_CPP)

/** @brief Effective X-coordinate gate. */
#define KYMO_EFFECTIVE_X (KYMO_ENABLED && KYMO_ENABLE_X)

/** @brief Effective Z-coordinate gate. */
#define KYMO_EFFECTIVE_Z (KYMO_ENABLED && KYMO_ENABLE_Z)

/** @brief Effective DATA timestamp gate. */
#define KYMO_EFFECTIVE_TIMESTAMP (KYMO_ENABLED && KYMO_ENABLE_TIMESTAMP)

/** @brief Effective encoder CRC gate. */
#define KYMO_EFFECTIVE_CRC (KYMO_ENABLED && KYMO_ENABLE_CRC)

/** @brief Effective MCU decoder gate. */
#define KYMO_EFFECTIVE_DECODER (KYMO_ENABLED && KYMO_ENABLE_DECODER)

/** @brief Effective parameter receiver gate (back channel). */
#define KYMO_EFFECTIVE_PARAMETERS (KYMO_ENABLED && KYMO_ENABLE_PARAMETERS)

/** @brief Effective live-variable gate (back channel, sampling by address). */
#define KYMO_EFFECTIVE_WATCH (KYMO_ENABLED && KYMO_ENABLE_WATCH)

/** @brief Effective trace recorder gate. */
#define KYMO_EFFECTIVE_TRACE (KYMO_ENABLED && KYMO_ENABLE_TRACE)

/** @brief Effective trace ISR gate. */
#define KYMO_EFFECTIVE_TRACE_ISR (KYMO_EFFECTIVE_TRACE && KYMO_ENABLE_TRACE_ISR)

/** @brief Effective scheduler trace gate. */
#define KYMO_EFFECTIVE_TRACE_SCHEDULER \
    (KYMO_EFFECTIVE_TRACE_ISR && KYMO_ENABLE_TRACE_SCHEDULER)

/** @brief Effective flow trace gate. */
#define KYMO_EFFECTIVE_TRACE_FLOW (KYMO_EFFECTIVE_TRACE && KYMO_ENABLE_TRACE_FLOW)

/** @brief Effective stack-pointer sampling gate. */
#define KYMO_EFFECTIVE_TRACE_STACK_POINTER     (KYMO_EFFECTIVE_TRACE && KYMO_ENABLE_TRACE_STACK_POINTER)

/** @brief Current stack address as uint32_t; override for compilers without the builtin.
 * @details Platform independent by default: the frame address of the recording
 * function sits a constant distance from the caller's stack pointer, which is all
 * a depth comparison needs.
 * @par Usage
 * #define KYMO_TRACE_READ_STACK_POINTER() my_read_sp()
 */
#if KYMO_EFFECTIVE_TRACE_STACK_POINTER && !defined(KYMO_TRACE_READ_STACK_POINTER)
#if defined(__GNUC__) || defined(__clang__)
#define KYMO_TRACE_READ_STACK_POINTER() ((uint32_t)(uintptr_t)__builtin_frame_address(0))
#else
#error "KYMO_ENABLE_TRACE_STACK_POINTER needs KYMO_TRACE_READ_STACK_POINTER() on this compiler"
#endif
#endif

#endif /* KYMO_CONFIG_H */
