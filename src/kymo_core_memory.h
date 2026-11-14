/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_memory.h
 * @brief Complete application interface for the default Core memory feature.
 * @details Configuration is copied; referenced tables, buffers and callback contexts
 * must remain alive. No heap or application-owned dispatcher state is required.
 * @par Usage Enable the feature, initialize Core, initialize this feature, call Core Main.
 */
#ifndef KYMO_CORE_MEMORY_H
#define KYMO_CORE_MEMORY_H
#include "kymo_types.h"
#if KYMO_EFFECTIVE_MEMORY || defined(DOXYGEN)

#ifdef __cplusplus
extern "C"
{
#endif

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
 * @brief Largest frame KymoMemory_nextInfo produces.
 * @details Sync (2), descriptor, length, operation, channel, kind, total (4), name, CRC.
 * @par Usage
 * Declare uint8_t frame[KYMO_MEMORY_FRAME_MAX_SIZE] for KymoMemory_nextInfo.
 */
#define KYMO_MEMORY_FRAME_MAX_SIZE (12u + KYMO_MEMORY_NAME_MAX)

    /**
     * @brief One reported value.
     * @details Referenced, not copied: totalSizeBytes may be filled in or changed after
     * KymoMemory_init (for example once a task exists); the next MEMORY_INFO carries it.
     * @par Usage
     * { "wifi", 4096u, KYMO_MEMORY_STACK_PEAK, 223u }
     */
    typedef struct
    {
        /** @brief Name shown by the host, e.g. the task name; NULL means empty. */
        const char *pName;

        /** @brief Size of the stack or heap in bytes; 0 when unknown. */
        uint32_t totalSizeBytes;

        /** @brief KYMO_MEMORY_STACK_PEAK, KYMO_MEMORY_HEAP_FREE or KYMO_MEMORY_HEAP_MIN_FREE. */
        uint8_t kind;

        /** @brief DATA channel of the samples; unique, not used by measurements. */
        uint8_t channel;
    } KymoMemoryEntry;

    /**
     * @brief Reads one entry's current value.
     * @param[in] pContext Application context from pReadContext.
     * @param[in] entryIndex Index into the configured pEntriesAv table.
     * @param[out] pSizeBytes Value in bytes (convert words to bytes here).
     * @return 1 when bytes holds a value, 0 to skip this sample.
     * @par Usage
     * static uint8_t read(void *c, size_t i, uint32_t *b) { *b = stack_size[i] - 4u * uxTaskGetStackHighWaterMark(task[i]); return 1u; }
     */
    typedef uint8_t (*KymoMemoryReadFn)(void *pContext, size_t entryIndex, uint32_t *pSizeBytes);

    /**
     * @brief Application configuration for memory.
     * @details Callback fields precede contexts and data parameters.
     * @par Usage Zero-initialize, assign named fields, and keep referenced storage alive.
     */
    typedef struct
    {
        /** @brief Required reader of one memory measurement, in bytes. */
        KymoMemoryReadFn readBytesCbk;

        /** @brief Read callback context. */
        void *pReadContext;

        /** @brief Persistent memory-entry table. */
        const KymoMemoryEntry *pEntriesAv;

        /** @brief Number of entries, 1..KYMO_MAX_MEMORY_ENTRIES. */
        size_t entryCount;

        /** @brief Sample interval in milliseconds, nonzero. */
        uint16_t samplePeriodMs;

        /** @brief Description repeat interval in milliseconds; zero disables repetition. */
        uint16_t infoPeriodMs;
    } KymoCoreMemoryConfig;
#if KYMO_EFFECTIVE_CORE || defined(DOXYGEN)

    /**
     * @brief Initialize memory and activate its processing in Core Main.
     * @details Call once after Core Init and before the first Main. Setup errors latch
     * a Core fault; late calls are rejected without altering the running Core.
     * No callbacks are invoked periodically until Core Main runs. Not ISR-safe.
     * @param[in] pConfig Persistent referenced data and callbacks; NULL is invalid.
     * @return OK, BAD_CONFIG, REJECTED, or an existing Core error.
     * @par Usage Check success before entering the common main loop.
     */
    KymoStatus KymoCoreMemory_init(const KymoCoreMemoryConfig *pConfig);

    /**
     * @brief Request another complete round of memory descriptions.
     * @details No effect before successful Init. Main transmits the descriptions later.
     * @par Usage Call from the Core task when the host needs refreshed labels.
     */
    void KymoCoreMemory_requestInfo(void);
#endif
#ifdef __cplusplus
}
#endif
#endif
#endif
