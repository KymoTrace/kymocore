/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_watch.h
 * @defgroup kymo_watch Live variables: the host samples memory by address
 * @{
 * @brief Sample global or static variables that the host picked from the ELF file.
 *
 * @details The host (KymoStudio) reads names, addresses and types of variables from
 * the ELF file of the running firmware and asks the device to sample some of them.
 * The device reads each chosen variable with exactly its width at the requested
 * period and the application sends the values as ordinary DATA frames, with its
 * own timestamp. Nothing is ever written through this module.
 *
 * Safety: the firmware declares which memory ranges may be read (KymoWatchRange).
 * A request is accepted only when the variable lies completely inside one range and
 * is aligned to its size; every request is answered with an acknowledgement, so the
 * host never assumes success. A build ID lets the host check that its ELF file
 * matches the running firmware (see PROTOCOL.md, "Live variables").
 *
 * Messages (control descriptor KYMO_DESCRIPTOR_PARAM, mandatory CRC): WATCH_INFO
 * stops every slot and asks for capabilities, WATCH_INFO_REPLY answers it,
 * WATCH_SET starts or stops one slot, WATCH_ACK reports the result.
 *
 * Cost: KymoWatch_Feed does constant work per byte; KymoWatch_NextSample checks at
 * most KYMO_MAX_WATCHES slots and reads one variable per call. No heap, no SDK
 * access: the application reads its transport, owns the clock and sends frames.
 * Nothing is compiled unless KYMO_ENABLE_WATCH=1.
 *
 * Atomicity: aligned reads of 1, 2 and 4 bytes are single-copy atomic on 32-bit
 * MCUs. An 8-byte (f64) read may tear while another task writes the variable;
 * install a KymoWatchReadFn that locks if that matters. The same callback lets a
 * host-side build (64-bit PC) map 32-bit wire addresses to real memory.
 *
 * Concurrency: not thread-safe. Call Feed, NextReply and NextSample from one task.
 * KymoParam and KymoWatch may be fed the same received bytes; each ignores the
 * other's messages.
 * @par Usage
 * @code{.c}
 * extern uint8_t _data_start, _bss_end;   // from the linker script
 * static KymoWatchRange ranges[1];
 * static KymoWatch watch;
 * ranges[0].start = (uint32_t)(uintptr_t)&_data_start;
 * ranges[0].length = (uint32_t)((uintptr_t)&_bss_end - (uintptr_t)&_data_start);
 * KymoWatchConfig config = { ranges, 1u, build_id, 16u, nullptr, nullptr };
 * KymoWatch_Init(&watch, &config);
 * // In the application loop:
 * KymoWatch_Feed(&watch, rx, rx_length);
 * while ((n = KymoWatch_NextReply(&watch, frame, sizeof(frame))) != 0u) { send(frame, n); }
 * while (KymoWatch_NextSample(&watch, now_ms, &point) == KYMO_OK) { add timestamp, encode, send; }
 * @endcode
 */
#ifndef KYMO_WATCH_H
/**
 * @brief Include guard for kymo_watch.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include KymoCore.h normally; the compiler manages this guard.
 */
#define KYMO_WATCH_H

#include <stddef.h>
#include <stdint.h>
#include "../kymo_types.h"
#include "../telemetry/kymo_protocol.h"
#include "kymo_control_rx.h"

#if KYMO_EFFECTIVE_WATCH || defined(DOXYGEN)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Operation: host stops every slot and asks for capabilities.
 * @details Payload is this single byte. The device answers with WATCH_INFO_REPLY.
 * @par Usage
 * Sent by the host; handled inside KymoWatch_Feed.
 */
#define KYMO_WATCH_OP_INFO 0x03u

/**
 * @brief Operation: device reports slots, minimum period, build ID and ranges.
 * @details Payload: op, slots (u8), min_period_ms (u16 LE), id_length (u8), build
 * ID bytes, range_count (u8), then start and length (u32 LE) per range.
 * @par Usage
 * Produced by KymoWatch_NextReply.
 */
#define KYMO_WATCH_OP_INFO_REPLY 0x04u

/**
 * @brief Operation: host starts or stops one slot.
 * @details Payload: op, slot (u8), channel (u8), address (u32 LE), type (u8),
 * period_ms (u16 LE). Period 0 stops the slot.
 * @par Usage
 * Sent by the host; handled inside KymoWatch_Feed.
 */
#define KYMO_WATCH_OP_SET 0x05u

/**
 * @brief Operation: device reports the result of one WATCH_SET.
 * @details Payload: op, slot (u8), status (u8, KYMO_WATCH_STATUS_*).
 * @par Usage
 * Produced by KymoWatch_NextReply.
 */
#define KYMO_WATCH_OP_ACK 0x06u

/** @brief Variable type: unsigned 8-bit integer. */
#define KYMO_WATCH_TYPE_U8 1u
/** @brief Variable type: signed 8-bit integer. */
#define KYMO_WATCH_TYPE_I8 2u
/** @brief Variable type: unsigned 16-bit integer. */
#define KYMO_WATCH_TYPE_U16 3u
/** @brief Variable type: signed 16-bit integer. */
#define KYMO_WATCH_TYPE_I16 4u
/** @brief Variable type: unsigned 32-bit integer (above 2^24 precision is lost). */
#define KYMO_WATCH_TYPE_U32 5u
/** @brief Variable type: signed 32-bit integer (beyond ±2^24 precision is lost). */
#define KYMO_WATCH_TYPE_I32 6u
/** @brief Variable type: IEEE-754 binary32. */
#define KYMO_WATCH_TYPE_F32 7u
/** @brief Variable type: IEEE-754 binary64, sent as binary32. */
#define KYMO_WATCH_TYPE_F64 8u
/** @brief Variable type: one byte, nonzero is sent as 1. */
#define KYMO_WATCH_TYPE_BOOL 9u

/** @brief Acknowledgement: request applied. */
#define KYMO_WATCH_STATUS_ACCEPTED 0u
/** @brief Acknowledgement: slot number not below KYMO_MAX_WATCHES. */
#define KYMO_WATCH_STATUS_BAD_SLOT 1u
/** @brief Acknowledgement: variable not completely inside one allowed range. */
#define KYMO_WATCH_STATUS_OUT_OF_RANGE 2u
/** @brief Acknowledgement: address not aligned to the type size. */
#define KYMO_WATCH_STATUS_MISALIGNED 3u
/** @brief Acknowledgement: unknown type code. */
#define KYMO_WATCH_STATUS_BAD_TYPE 4u
/** @brief Acknowledgement: period below KYMO_WATCH_MIN_PERIOD_MS. */
#define KYMO_WATCH_STATUS_BAD_PERIOD 5u

/**
 * @brief Longest build ID in bytes.
 * @details KymoWatch_Init rejects longer IDs. 20 bytes hold a SHA-1 GNU build ID.
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
 * @brief Largest frame KymoWatch_NextReply produces (a full WATCH_INFO_REPLY).
 * @details Sync (2), descriptor, length, 6 fixed payload bytes, build ID, range
 * count, 8 bytes per range and CRC.
 * @par Usage
 * Declare uint8_t frame[KYMO_WATCH_FRAME_MAX_SIZE] for KymoWatch_NextReply.
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
        uint32_t start;
        /** @brief Number of readable bytes from start. */
        uint32_t length;
    } KymoWatchRange;

    /**
     * @brief Optional memory reader.
     *
     * @details Copies @p size bytes from device address @p address into @p output in
     * the device's own byte order. Use it to lock around 8-byte reads or to map wire
     * addresses on a host build. Called only for checked, aligned addresses.
     * @param[in] context Application context from KymoWatchConfig.
     * @param[in] address Device address inside an allowed range.
     * @param[in] size 1, 2, 4 or 8.
     * @param[out] output Destination of at least @p size bytes.
     * @return 1 when the bytes were read, 0 to skip this sample.
     * @par Usage
     * static uint8_t read_locked(void *c, uint32_t a, uint8_t n, uint8_t *o) { lock(); memcpy(o, (void *)(uintptr_t)a, n); unlock(); return 1u; }
     */
    typedef uint8_t (*KymoWatchReadFn)(void *context, uint32_t address, uint8_t size,
                                       uint8_t *output);

    /**
     * @brief Configuration copied by KymoWatch_Init.
     *
     * @details ranges and build_id are referenced, not copied; they must outlive the
     * KymoWatch.
     * @par Usage
     * KymoWatchConfig config = { ranges, 1u, id, 16u, nullptr, nullptr };
     */
    typedef struct
    {
        /** @brief Allowed memory ranges; 1..KYMO_WATCH_MAX_RANGES entries. */
        const KymoWatchRange *ranges;
        /** @brief Number of ranges. */
        size_t range_count;
        /** @brief Build ID reported to the host; NULL when build_id_length is 0. */
        const uint8_t *build_id;
        /** @brief Build ID length, 0..KYMO_WATCH_BUILD_ID_MAX; 0 means unknown. */
        size_t build_id_length;
        /** @brief Optional reader; NULL reads memory directly with the type's width. */
        KymoWatchReadFn read;
        /** @brief Context passed to read. */
        void *read_context;
    } KymoWatchConfig;

    /**
     * @brief State of one slot. Treat as read-only.
     * @par Usage
     * Inspect watch.slots[i] for diagnostics.
     */
    typedef struct
    {
        /** @brief Device address of the variable. */
        uint32_t address;
        /** @brief Time of the next sample in ms (valid unless fresh). */
        uint32_t due_ms;
        /** @brief Sampling period in ms. */
        uint16_t period_ms;
        /** @brief Wire channel of the samples. */
        uint8_t channel;
        /** @brief KYMO_WATCH_TYPE_*; 0 when the slot is stopped. */
        uint8_t type;
        /** @brief 1 until the first sample after a WATCH_SET. */
        uint8_t fresh;
    } KymoWatchSlot;

    /**
     * @brief Live-variable state: configuration, reassembly, slots, pending replies.
     *
     * @details Treat all fields as read-only after KymoWatch_Init; the counters are
     * diagnostics and wrap modulo 2^32.
     * @par Usage
     * Declare one static KymoWatch per receiving transport.
     */
    typedef struct
    {
        /** @brief Copied configuration. */
        KymoWatchConfig config;
        /** @brief Frame reassembly shared with the other back-channel modules. */
        KymoControlRx rx;
        /** @brief Sampling slots. */
        KymoWatchSlot slots[KYMO_MAX_WATCHES];
        /** @brief Status of the pending acknowledgement per slot. */
        uint8_t ack_status[KYMO_MAX_WATCHES];
        /** @brief One bit per slot: an acknowledgement is due. */
        uint8_t ack_pending[(KYMO_MAX_WATCHES + 7u) / 8u];
        /** @brief Slot number of a request with an invalid slot, answered once. */
        uint8_t bad_slot;
        /** @brief 1 while the BAD_SLOT acknowledgement for bad_slot is due. */
        uint8_t bad_slot_pending;
        /** @brief 1 while a WATCH_INFO_REPLY is due. */
        uint8_t info_due;
        /** @brief 1 after a successful KymoWatch_Init. */
        uint8_t ready;
        /** @brief Inclusive allowed channel range; default Init permits all IDs. */
        uint8_t first_channel;
        /** @brief Last allowed channel; restricted Init reserves a project range. */
        uint8_t last_channel;
        /** @brief Slot where the next acknowledgement search starts. */
        size_t ack_cursor;
        /** @brief Slot where the next sample search starts (round robin). */
        size_t sample_cursor;
        /** @brief WATCH_SET requests accepted. */
        uint32_t accepted;
        /** @brief WATCH_SET requests refused. */
        uint32_t rejected;
        /** @brief Frames discarded: bad CRC, wrong length or unknown operation. */
        uint32_t invalid;
        /** @brief Samples skipped because the reader returned 0. */
        uint32_t read_failures;
    } KymoWatch;

    /**
     * @brief Validate the configuration and reset all slots.
     *
     * @details Rejects NULL arguments, 0 or more than KYMO_WATCH_MAX_RANGES ranges,
     * empty or wrapping ranges, a build ID longer than KYMO_WATCH_BUILD_ID_MAX and a
     * NULL build ID with nonzero length. On failure the watch is unusable until a
     * successful Init.
     * @param[out] watch State to reset; NULL is rejected.
     * @param[in] config Configuration; NULL is rejected. Copied; referenced arrays must outlive @p watch.
     * @return KYMO_OK, or KYMO_BAD_CONFIG.
     * @par Usage
     * Call once during start-up, before the first KymoWatch_Feed.
     */
    KymoStatus KymoWatch_Init(KymoWatch *watch, const KymoWatchConfig *config);

    /** @brief Initialize Watch with an inclusive allowed DATA-channel range.
     * @details Requests outside this range receive OUT_OF_RANGE before any slot is
     * activated. Period-zero stop requests remain valid. Ordinary Init permits all
     * channels; this entry point is useful when several features share a stream.
     * @param[out] watch State to initialize; NULL is invalid.
     * @param[in] config Normal Watch configuration.
     * @param[in] first_channel First allowed channel.
     * @param[in] last_channel Last allowed channel; must not precede first_channel.
     * @return OK or BAD_CONFIG. Failure leaves a non-NULL watch uninitialized.
     * @par Usage Reserve the same channel range in the coordinating application.
     */
    KymoStatus KymoWatch_InitRestricted(KymoWatch *watch, const KymoWatchConfig *config,
                                        uint8_t first_channel, uint8_t last_channel);

    /**
     * @brief Consume received bytes and handle every complete live-variable request.
     *
     * @details Bytes may arrive in any split. DATA frames and other modules' control
     * messages are ignored without counting them as invalid.
     * @param[in,out] watch Initialized state; NULL or uninitialized input is ignored.
     * @param[in] bytes Received bytes; NULL is treated as empty.
     * @param[in] length Number of readable bytes.
     * @return Number of WATCH_INFO and WATCH_SET requests handled during this call.
     * @par Usage
     * Feed whatever a non-blocking read returned, then drain KymoWatch_NextReply.
     */
    size_t KymoWatch_Feed(KymoWatch *watch, const uint8_t *bytes, size_t length);

    /**
     * @brief Encode the next due reply: first a WATCH_INFO_REPLY, then acknowledgements.
     *
     * @details The frame is complete and CRC-protected; send it as is. Taking a reply
     * clears it, so send it in the same step.
     * @param[in,out] watch Initialized state; NULL yields zero.
     * @param[out] output Frame buffer; NULL yields zero.
     * @param[in] capacity Writable bytes; KYMO_WATCH_FRAME_MAX_SIZE fits every reply.
     * @return Frame length, or zero when none is due or the buffer is too small (the
     *         reply then stays due).
     * @par Usage
     * while ((n = KymoWatch_NextReply(&watch, frame, sizeof(frame))) != 0u) { send(frame, n); }
     */
    size_t KymoWatch_NextReply(KymoWatch *watch, uint8_t *output, size_t capacity);

    /**
     * @brief Take the next due sample.
     *
     * @details Checks the slots round robin and reads the first due variable. Fills id
     * (the requested channel) and value; flags and the other fields are zero, so the
     * application can add its timestamp. A slot sampled late is rescheduled from
     * @p now_ms, so a pause never causes a burst.
     * @param[in,out] watch Initialized state; NULL is rejected.
     * @param[in] now_ms Application clock in milliseconds (wrapping is fine).
     * @param[out] point Destination; NULL is rejected.
     * @return KYMO_OK with a sample, KYMO_IDLE if none is due, KYMO_BAD_CONFIG for NULL
     *         or uninitialized input.
     * @par Usage
     * while (KymoWatch_NextSample(&watch, now_ms, &point) == KYMO_OK) { send point; }
     */
    KymoStatus KymoWatch_NextSample(KymoWatch *watch, uint32_t now_ms, KymoDataPoint *point);

#ifdef __cplusplus
}
#endif

#endif /* KYMO_EFFECTIVE_WATCH */

/** @} */
#endif /* KYMO_WATCH_H */
