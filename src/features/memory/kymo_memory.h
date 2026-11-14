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
 * out right after KymoMemory_init, every infoPeriodMs and after
 * KymoMemory_requestInfo, so a host that connects later still learns the names.
 * No back channel is needed.
 *
 * @par Usage
 * @code
 * static KymoMemoryEntry entries[2] = {
 *     { "parser", 4096u, KYMO_MEMORY_STACK_PEAK, 220u },
 *     { "heap", 0u, KYMO_MEMORY_HEAP_FREE, 221u },
 * };
 * static uint8_t read(void *c, size_t i, uint32_t *bytes) { ...; return 1u; }
 * KymoCoreMemoryConfig config = { read, nullptr, entries, 2u, 500u, 5000u };
 * KymoMemory_init(&memory, &config);
 * // in the main loop:
 * while ((n = KymoMemory_nextInfo(&memory, now, frame, sizeof(frame))) != 0u) { send(frame, n); }
 * while (KymoMemory_nextSample(&memory, now, &point) == KYMO_OK) { send point; }
 * @endcode
 */
#ifndef KYMO_MEMORY_H
#define KYMO_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include "../../kymo_core_memory.h"
#include "../../communication/kymo_protocol.h"

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
 * Produced by KymoMemory_nextInfo; the host names the channel after it.
 */
#define KYMO_MEMORY_OP_INFO 0x07u

    /**
     * @brief Memory telemetry state. Treat as read-only.
     * @par Usage
     * static KymoMemory memory;
     */
    typedef struct
    {
        /** @brief Copy of the configuration. */
        KymoCoreMemoryConfig config;

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

        /** @brief 1 after a successful KymoMemory_init. */
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
    KymoStatus KymoMemory_init(KymoMemory *memory, const KymoCoreMemoryConfig *config);

    /**
     * @brief Sends every MEMORY_INFO again from the next KymoMemory_nextInfo call on.
     * @param[in,out] memory Initialized state; NULL is ignored.
     * @par Usage
     * Call when the host connects or after a transport restart.
     */
    void KymoMemory_requestInfo(KymoMemory *memory);

    /**
     * @brief Produces the next due MEMORY_INFO frame.
     * @param[in,out] memory Initialized state.
     * @param[in] now_ms Current time in ms.
     * @param[out] output Frame buffer.
     * @param[in] capacity Size of output; at least KYMO_MEMORY_FRAME_MAX_SIZE.
     * @return Frame length, or 0 when nothing is due or the buffer is too small (it stays due).
     * @par Usage
     * while ((n = KymoMemory_nextInfo(&memory, now, frame, sizeof(frame))) != 0u) { send(frame, n); }
     */
    size_t KymoMemory_nextInfo(KymoMemory *memory, uint32_t now_ms, uint8_t *output,
                               size_t capacity);

    /**
     * @brief Produces the next due sample as an ordinary DATA point.
     * @param[in,out] memory Initialized state.
     * @param[in] now_ms Current time in ms.
     * @param[out] point Channel and value in bytes; no timestamp is set.
     * @return KYMO_OK with a point, KYMO_IDLE when nothing is due, KYMO_BAD_CONFIG
     * for NULL arguments or an uninitialized state.
     * @par Usage
     * while (KymoMemory_nextSample(&memory, now, &point) == KYMO_OK) { send point; }
     */
    KymoStatus KymoMemory_nextSample(KymoMemory *memory, uint32_t now_ms, KymoDataPoint *point);

#ifdef __cplusplus
}
#endif

#endif /* KYMO_EFFECTIVE_MEMORY */

#endif /* KYMO_MEMORY_H */
