/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_watch.h
 * @brief Complete application interface for the default Core watch feature.
 * @details Configuration is copied; referenced tables, buffers and callback contexts
 * must remain alive. No heap or application-owned dispatcher state is required.
 * @par Usage Enable the feature, initialize Core, initialize this feature, call Core Main.
 */
#ifndef KYMO_CORE_WATCH_H
#define KYMO_CORE_WATCH_H
#include "kymo_types.h"
#if KYMO_EFFECTIVE_WATCH || defined(DOXYGEN)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Longest build ID in bytes.
 * @details KymoWatch_init rejects longer IDs. 20 bytes hold a SHA-1 GNU build ID.
 * @par Usage
 * Pass at most this many bytes, e.g. the first 16 of an ESP32 app_elf_sha256.
 */
#define KYMO_WATCH_BUILD_ID_MAX 20u

/**
 * @brief Most memory ranges one KymoWatch reports and checks.
 * @details Limited by the 64-byte control payload together with the build ID.
 * @par Usage
 * Typically one range for .data/.bss, perhaps a second for external RAM.
 */
#define KYMO_WATCH_MAX_RANGES 4u

/**
 * @brief Largest frame KymoWatch_nextReply produces (a full WATCH_INFO_REPLY).
 * @details Sync (2), descriptor, length, 6 fixed payload bytes, build ID, range
 * count, 8 bytes per range and CRC.
 * @par Usage
 * Declare uint8_t frame[KYMO_WATCH_FRAME_MAX_SIZE] for KymoWatch_nextReply.
 */
#define KYMO_WATCH_FRAME_MAX_SIZE (11u + KYMO_WATCH_BUILD_ID_MAX + (8u * KYMO_WATCH_MAX_RANGES))

    /**
     * @brief One memory range the host may read.
     *
     * @details Addresses are the device's own 32-bit addresses as they appear in the
     * ELF file. A range must not be empty and must not wrap past 2^32.
     * @par Usage
     * { (uint32_t)(uintptr_t)&_data_start, data_and_bss_length }
     */
    typedef struct
    {
        /** @brief First readable address. */
        uint32_t startAddress;

        /** @brief Number of readable bytes from start. */
        uint32_t sizeBytes;
    } KymoWatchRange;

    /**
     * @brief Optional memory reader.
     *
     * @details Copies @p sizeBytes bytes from device address @p address into @p pOutputAv in
     * the device's own byte order. Use it to lock around 8-byte reads or to map wire
     * addresses on a host build. Called only for checked, aligned addresses.
     * @param[in] pContext Application context supplied through pReadContext.
     * @param[in] address Device address inside an allowed range.
     * @param[in] sizeBytes 1, 2, 4 or 8.
     * @param[out] pOutputAv Destination of at least @p sizeBytes bytes.
     * @return 1 when the bytes were read, 0 to skip this sample.
     * @par Usage
     * static uint8_t read_locked(void *c, uint32_t a, uint8_t n, uint8_t *o) { lock(); memcpy(o, (void *)(uintptr_t)a, n); unlock(); return 1u; }
     */
    typedef uint8_t (*KymoWatchReadFn)(void *pContext, uint32_t address, uint8_t sizeBytes,
                                       uint8_t *pOutputAv);

    /**
     * @brief Application configuration for watch.
     * @details Callback fields precede contexts and data parameters.
     * @par Usage Zero-initialize, assign named fields, and keep referenced storage alive.
     */
    typedef struct
    {
        /** @brief Optional safe reader; NULL uses direct device-address reads. */
        KymoWatchReadFn readValueCbk;

        /** @brief Read callback context. */
        void *pReadContext;

        /** @brief Persistent allowed address ranges. */
        const KymoWatchRange *pRangesAv;

        /** @brief Number of allowed ranges, 1..KYMO_WATCH_MAX_RANGES. */
        size_t rangeCount;

        /** @brief Persistent build identifier bytes. */
        const uint8_t *pBuildIdAv;

        /** @brief Build identifier size, at most KYMO_WATCH_BUILD_ID_MAX. */
        size_t buildIdSizeBytes;

        /** @brief First reserved DATA channel, inclusive. */
        uint8_t firstChannel;

        /** @brief Last reserved DATA channel, inclusive. */
        uint8_t lastChannel;
    } KymoCoreWatchConfig;
#if KYMO_EFFECTIVE_CORE || defined(DOXYGEN)

    /**
     * @brief Initialize watch and activate its processing in Core Main.
     * @details Call once after Core Init and before the first Main. Setup errors latch
     * a Core fault; late calls are rejected without altering the running Core.
     * No callbacks are invoked periodically until Core Main runs. Not ISR-safe.
     * @param[in] pConfig Persistent referenced data and callbacks; NULL is invalid.
     * @return OK, BAD_CONFIG, REJECTED, or an existing Core error.
     * @par Usage Check success before entering the common main loop.
     */
    KymoStatus KymoCoreWatch_init(const KymoCoreWatchConfig *pConfig);

#endif
#ifdef __cplusplus
}
#endif
#endif
#endif
