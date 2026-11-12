/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_memory.h
 * @brief Memory telemetry: stack peaks and heap levels as named curves.
 *
 * @details The firmware lists what it wants to report (task stacks, heaps) and
 * reads the values in a callback; KymoMemory schedules the samples and describes
 * each entry to the host. Platform independent: the callback asks the RTOS or
 * allocator (uxTaskGetStackHighWaterMark, xPortGetFreeHeapSize, heap_caps_*,
 * mallinfo); KymoCore never calls them itself.
 *
 * Every entry has its own DATA channel. MEMORY_INFO frames (operation 0x07 on the
 * control descriptor 0x60) tell the host its name, kind and total size. They go
 * out right after KymoMemory_Init, every info_period_ms and after
 * KymoMemory_RequestInfo, so a host that connects later still learns the names.
 * No back channel is needed.
 *
 * @par Usage
 * @code
 * static KymoMemoryEntry entries[2] = {
 *     { "parser", 4096u, KYMO_MEMORY_STACK_PEAK, 220u },
 *     { "heap", 0u, KYMO_MEMORY_HEAP_FREE, 221u },
 * };
 * static uint8_t read(void *c, size_t i, uint32_t *bytes) { ...; return 1u; }
 * KymoMemoryConfig config = { entries, 2u, 500u, 5000u, read, nullptr };
 * KymoMemory_Init(&memory, &config);
 * // in the main loop:
 * while ((n = KymoMemory_NextInfo(&memory, now, frame, sizeof(frame))) != 0u) { send(frame, n); }
 * while (KymoMemory_NextSample(&memory, now, &point) == KYMO_OK) { send point; }
 * @endcode
 */
#ifndef KYMO_MEMORY_H
#define KYMO_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include "../kymo_types.h"
#include "kymo_protocol.h"

#if KYMO_EFFECTIVE_MEMORY || defined(DOXYGEN)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Operation: device describes one memory entry.
 * @details Payload after the operation byte: channel u8, kind u8, total u32 LE,
 * then the name (0..KYMO_MEMORY_NAME_MAX bytes UTF-8, the rest of the payload).
 * @par Usage
 * Produced by KymoMemory_NextInfo; the host names the channel after it.
 */
#define KYMO_MEMORY_OP_INFO 0x07u

/**
 * @brief Kind: peak stack use of a task in bytes (stack size minus watermark).
 * @par Usage
 * { "parser", 4096u, KYMO_MEMORY_STACK_PEAK, 220u }
 */
#define KYMO_MEMORY_STACK_PEAK 1u
/**
 * @brief Kind: free heap bytes now.
 * @par Usage
 * { "heap", total, KYMO_MEMORY_HEAP_FREE, 221u }
 */
#define KYMO_MEMORY_HEAP_FREE 2u
/**
 * @brief Kind: lowest free heap bytes since start.
 * @par Usage
 * { "heap", total, KYMO_MEMORY_HEAP_MIN_FREE, 222u }
 */
#define KYMO_MEMORY_HEAP_MIN_FREE 3u

/**
 * @brief Longest entry name in bytes; longer names are cut.
 * @par Usage
 * Keep task names short, as FreeRTOS does (configMAX_TASK_NAME_LEN).
 */
#define KYMO_MEMORY_NAME_MAX 16u

/**
 * @brief Largest frame KymoMemory_NextInfo produces.
 * @details Sync (2), descriptor, length, operation, channel, kind, total (4), name, CRC.
 * @par Usage
 * Declare uint8_t frame[KYMO_MEMORY_FRAME_MAX_SIZE] for KymoMemory_NextInfo.
 */
#define KYMO_MEMORY_FRAME_MAX_SIZE (12u + KYMO_MEMORY_NAME_MAX)

    /**
     * @brief One reported value.
     * @details Referenced, not copied: total_bytes may be filled in or changed after
     * KymoMemory_Init (for example once a task exists); the next MEMORY_INFO carries it.
     * @par Usage
     * { "wifi", 4096u, KYMO_MEMORY_STACK_PEAK, 223u }
     */
    typedef struct
    {
        /** @brief Name shown by the host, e.g. the task name; NULL means empty. */
        const char *name;
        /** @brief Size of the stack or heap in bytes; 0 when unknown. */
        uint32_t total_bytes;
        /** @brief KYMO_MEMORY_STACK_PEAK, KYMO_MEMORY_HEAP_FREE or KYMO_MEMORY_HEAP_MIN_FREE. */
        uint8_t kind;
        /** @brief DATA channel of the samples; unique, not used by measurements. */
        uint8_t channel;
    } KymoMemoryEntry;

    /**
     * @brief Reads one entry's current value.
     * @param[in] context Application context from KymoMemoryConfig.
     * @param[in] index Entry index in KymoMemoryConfig::entries.
     * @param[out] bytes Value in bytes (convert words to bytes here).
     * @return 1 when bytes holds a value, 0 to skip this sample.
     * @par Usage
     * static uint8_t read(void *c, size_t i, uint32_t *b) { *b = stack_size[i] - 4u * uxTaskGetStackHighWaterMark(task[i]); return 1u; }
     */
    typedef uint8_t (*KymoMemoryReadFn)(void *context, size_t index, uint32_t *bytes);

    /**
     * @brief Configuration copied by KymoMemory_Init; entries are referenced.
     * @par Usage
     * KymoMemoryConfig config = { entries, 3u, 500u, 5000u, read, nullptr };
     */
    typedef struct
    {
        /** @brief Reported entries; 1..KYMO_MAX_MEMORY_ENTRIES. */
        const KymoMemoryEntry *entries;
        /** @brief Number of entries. */
        size_t count;
        /** @brief Sampling period of every entry in ms (1..60000). */
        uint16_t period_ms;
        /** @brief Period of the repeated MEMORY_INFO round in ms; 0 sends it only on start and request. */
        uint16_t info_period_ms;
        /** @brief Required value reader. */
        KymoMemoryReadFn read;
        /** @brief Context passed to read. */
        void *context;
    } KymoMemoryConfig;

    /**
     * @brief Memory telemetry state. Treat as read-only.
     * @par Usage
     * static KymoMemory memory;
     */
    typedef struct
    {
        /** @brief Copy of the configuration. */
        KymoMemoryConfig config;
        /** @brief Time of the next sampling round in ms. */
        uint32_t sample_due_ms;
        /** @brief Time of the next MEMORY_INFO round in ms. */
        uint32_t info_due_ms;
        /** @brief Next entry of the running sampling round; count when idle. */
        size_t sample_next;
        /** @brief Next entry of the running MEMORY_INFO round; count when idle. */
        size_t info_next;
        /** @brief Samples skipped because read returned 0. */
        uint32_t read_failures;
        /** @brief 1 until the first sampling round started. */
        uint8_t fresh;
        /** @brief 1 after a successful KymoMemory_Init. */
        uint8_t ready;
    } KymoMemory;

    /**
     * @brief Checks the configuration and schedules the first MEMORY_INFO round.
     * @param[out] memory State to initialize.
     * @param[in] config Entries, periods and reader; copied.
     * @return KYMO_OK, or KYMO_BAD_CONFIG for NULL arguments, 0 or too many entries,
     * an unknown kind, a period of 0 or above 60000 ms, or a missing reader.
     * @par Usage
     * Call once during start-up.
     */
    KymoStatus KymoMemory_Init(KymoMemory *memory, const KymoMemoryConfig *config);

    /**
     * @brief Sends every MEMORY_INFO again from the next KymoMemory_NextInfo call on.
     * @param[in,out] memory Initialized state; NULL is ignored.
     * @par Usage
     * Call when the host connects or after a transport restart.
     */
    void KymoMemory_RequestInfo(KymoMemory *memory);

    /**
     * @brief Produces the next due MEMORY_INFO frame.
     * @param[in,out] memory Initialized state.
     * @param[in] now_ms Current time in ms.
     * @param[out] output Frame buffer.
     * @param[in] capacity Size of output; at least KYMO_MEMORY_FRAME_MAX_SIZE.
     * @return Frame length, or 0 when nothing is due or the buffer is too small (it stays due).
     * @par Usage
     * while ((n = KymoMemory_NextInfo(&memory, now, frame, sizeof(frame))) != 0u) { send(frame, n); }
     */
    size_t KymoMemory_NextInfo(KymoMemory *memory, uint32_t now_ms, uint8_t *output,
                               size_t capacity);

    /**
     * @brief Produces the next due sample as an ordinary DATA point.
     * @param[in,out] memory Initialized state.
     * @param[in] now_ms Current time in ms.
     * @param[out] point Channel and value in bytes; no timestamp is set.
     * @return KYMO_OK with a point, KYMO_IDLE when nothing is due, KYMO_BAD_CONFIG
     * for NULL arguments or an uninitialized state.
     * @par Usage
     * while (KymoMemory_NextSample(&memory, now, &point) == KYMO_OK) { send point; }
     */
    KymoStatus KymoMemory_NextSample(KymoMemory *memory, uint32_t now_ms, KymoDataPoint *point);

#ifdef __cplusplus
}
#endif

#endif /* KYMO_EFFECTIVE_MEMORY */

#endif /* KYMO_MEMORY_H */
