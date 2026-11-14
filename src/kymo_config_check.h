/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */

/**
 * @file kymo_config_check.h
 * @brief Validates the settings of kymo_config.h and derives KYMO_EFFECTIVE_*.
 * @details Internal part of kymo_config.h; nothing here is a setting. Rejects
 * invalid values and unmet dependencies with `#error`, then publishes the
 * KYMO_EFFECTIVE_* gates that the library and applications test.
 * @par Usage
 * Do not include directly; kymo_config.h includes it after all settings.
 */
#ifndef KYMO_CONFIG_CHECK_H
#define KYMO_CONFIG_CHECK_H

#ifndef KYMO_CONFIG_H
#error "Include kymo_config.h instead of kymo_config_check.h"
#endif

#if defined(__cplusplus) && (__cplusplus < 201103L)
#error "KymoCore requires C++11 or newer"
#endif

/* ---- Validation --------------------------------------------------------- */

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

#if !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLED) ||                \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_CORE) ||            \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_RUNTIME) ||         \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_ENDPOINT) ||        \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TX) ||              \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_CPP) ||             \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_X) ||               \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_Z) ||               \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TIMESTAMP) ||       \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_CRC) ||             \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_CRC_TABLE) ||       \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_DECODER) ||         \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_PARAMETERS) ||      \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_WATCH) ||           \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_MEMORY) ||          \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE) ||           \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE_ISR) ||       \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE_SCHEDULER) || \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE_FLOW) ||      \
    !KYMO_DETAIL_PASTE(KYMO_DETAIL_VALID_, KYMO_ENABLE_TRACE_STACK_POINTER)
#error "Kymo feature switches must be 0 or 1 (literal tokens only)"
#endif

#undef KYMO_DETAIL_PASTE
#undef KYMO_DETAIL_PASTE_I
#undef KYMO_DETAIL_VALID_0
#undef KYMO_DETAIL_VALID_1
#undef KYMO_DETAIL_PROFILE_2

#if KYMO_ENABLED && KYMO_ENABLE_ENDPOINT && !KYMO_ENABLE_TX && !KYMO_ENABLE_CORE
#error "KYMO_ENABLE_ENDPOINT requires KYMO_ENABLE_TX=1"
#endif

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

#if (KYMO_MAX_MEMORY_ENTRIES < 1u) || (KYMO_MAX_MEMORY_ENTRIES > 32u)
#error "KYMO_MAX_MEMORY_ENTRIES must be from 1 through 32"
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

/** @brief Effective default Core gate. @par Usage Guard public Core callers. */
#define KYMO_EFFECTIVE_CORE (KYMO_ENABLED && KYMO_ENABLE_CORE)

/** @brief Dispatcher needed by either explicit instances or the default Core. */
#define KYMO_EFFECTIVE_ENDPOINT (KYMO_ENABLED && (KYMO_ENABLE_ENDPOINT || KYMO_ENABLE_CORE))

/**
 * @brief Effective shared transport gate, respecting the global switch.
 * @par Usage Gate code that uses KymoTx with this macro.
 */
#define KYMO_EFFECTIVE_TX (KYMO_ENABLED && (KYMO_ENABLE_TX || KYMO_ENABLE_CORE))

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

/** @brief Effective memory telemetry gate. */
#define KYMO_EFFECTIVE_MEMORY (KYMO_ENABLED && KYMO_ENABLE_MEMORY)

/** @brief Effective trace recorder gate. */
#define KYMO_EFFECTIVE_TRACE (KYMO_ENABLED && KYMO_ENABLE_TRACE)

/** @brief Effective trace ISR gate. */
#define KYMO_EFFECTIVE_TRACE_ISR (KYMO_EFFECTIVE_TRACE && KYMO_ENABLE_TRACE_ISR)

/** @brief Effective scheduler trace gate. */
#define KYMO_EFFECTIVE_TRACE_SCHEDULER (KYMO_EFFECTIVE_TRACE_ISR && KYMO_ENABLE_TRACE_SCHEDULER)

/** @brief Effective flow trace gate. */
#define KYMO_EFFECTIVE_TRACE_FLOW (KYMO_EFFECTIVE_TRACE && KYMO_ENABLE_TRACE_FLOW)

/** @brief Effective stack-pointer sampling gate. */
#define KYMO_EFFECTIVE_TRACE_STACK_POINTER (KYMO_EFFECTIVE_TRACE && KYMO_ENABLE_TRACE_STACK_POINTER)

/**
 * @brief Current stack address as uint32_t; override for compilers without the builtin.
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

#endif /* KYMO_CONFIG_CHECK_H */
