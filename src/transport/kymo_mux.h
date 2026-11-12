/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */

/**
 * @file kymo_mux.h
 * @brief Single-writer transport mux: trace events, telemetry DATA and admin records on one link.
 *
 * @details The mux owns one frame buffer and fairly rotates three lanes:
 * administrative records (session metadata, dictionary, health, session end),
 * trace event batches drained from a KymoTraceRecorder, and telemetry DATA frames.
 * Every frame is wrapped in a trace protocol v2 envelope. Short writes and busy
 * transports are resumed; KymoMux_Service performs at most one frame attempt.
 *
 * To share the link with the telemetry runtime, route Kymo_Main output into a
 * KymoMuxDataSlot and let the mux pull it:
 * @code{.c}
 * static KymoMuxDataSlot slot;
 * KymoMuxDataSlot_Init(&slot);
 * runtimeConfig.write = KymoMuxDataSlot_Write;
 * runtimeConfig.busy = KymoMuxDataSlot_IsBusy;
 * runtimeConfig.transport_user = &slot;
 * muxConfig.next_data = KymoMuxDataSlot_NextData;
 * muxConfig.data_user = &slot;
 * @endcode
 * Available with KYMO_ENABLE_TRACE=1.
 * @par Usage
 * KymoMux_Init once, then call KymoMux_Service (and Kymo_Main) from the cyclic task.
 */
#ifndef KYMO_MUX_H
#define KYMO_MUX_H

#include "../kymo_types.h"
#include "../trace/kymo_trace.h"
#include "../telemetry/kymo_protocol.h"

#if KYMO_EFFECTIVE_TRACE || defined(DOXYGEN)

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Accept a prefix of a pending mux frame.
     * @param[in,out] user transport_user from KymoMuxConfig.
     * @param[in] bytes Readable frame bytes owned by the mux.
     * @param[in] length Offered byte count, up to KYMO_TRACE_FRAME_MAX.
     * @return Accepted count in 0..length; larger values latch KYMO_IO_ERROR.
     */
    typedef size_t (*KymoMuxWriteFn)(void *user, const uint8_t *bytes, size_t length);

    /**
     * @brief Supply the next complete telemetry DATA frame, if any.
     * @param[in,out] user data_user from KymoMuxConfig.
     * @param[out] frame Destination buffer.
     * @param[in] capacity Destination capacity.
     * @return Frame length, or zero when no frame is available.
     */
    typedef size_t (*KymoMuxNextDataFn)(void *user, uint8_t *frame, size_t capacity);

    /**
     * @brief Mux wiring: required fields first, optional fields may stay zero.
     * @details Copied by KymoMux_Init. Start from a zero-initialized object.
     */
    typedef struct KymoMuxConfig
    {
        /** @brief Required: initialized recorder whose events are transmitted. */
        KymoTraceRecorder *recorder;
        /** @brief Required: transport write callback. */
        KymoMuxWriteFn write;
        /** @brief Optional: transport buffer-ownership callback; NULL means write copies. */
        KymoBusyFn busy;
        /** @brief Optional: telemetry DATA source, e.g. KymoMuxDataSlot_NextData. */
        KymoMuxNextDataFn next_data;
        /** @brief Optional: context for write and busy. */
        void *transport_user;
        /** @brief Optional: context for next_data, e.g. a KymoMuxDataSlot. */
        void *data_user;
        /** @brief Optional: largest frame the link carries, 64..240; zero selects 240. */
        uint16_t max_frame_size;
        /** @brief Optional: recorder ticks between health records; zero disables them. */
        uint32_t health_period_ticks;
        /** @brief Optional: events per batch frame, 1..6; zero selects 6. */
        uint8_t max_batch_events;
    } KymoMuxConfig;

    /** @brief Caller-owned mux state; fields are internal and exposed only for static allocation. */
    typedef struct KymoMux
    {
        KymoMuxConfig config;                /**< Validated configuration copy. */
        uint8_t frame[KYMO_TRACE_FRAME_MAX]; /**< Current frame; stays unchanged while busy. */
        uint8_t admin_record[KYMO_MUX_ADMIN_RECORD_MAX]; /**< Queued admin record plus CRC-32. */
        size_t frame_length;                             /**< Current frame length. */
        size_t frame_offset;          /**< Accepted bytes of the current frame. */
        size_t last_frame_length;     /**< Length of the last completed frame. */
        uint32_t admin_total;         /**< Queued admin record length. */
        uint32_t admin_offset;        /**< Admin bytes already sent. */
        uint32_t admin_generation;    /**< Generation of the queued record. */
        uint32_t transfer_id;         /**< Admin transfer counter. */
        uint32_t packet_seq;          /**< Envelope sequence counter. */
        uint64_t next_health_ticks;   /**< Recorder tick of the next health record. */
        uint16_t pending_admin_bytes; /**< Admin bytes in the current frame. */
        uint8_t admin_kind;           /**< Payload kind of the queued record. */
        uint8_t admin_pending;        /**< Nonzero while an admin record is queued. */
        uint8_t pending;              /**< Nonzero while a frame is in flight. */
        uint8_t pending_kind;         /**< Lane of the current frame. */
        uint8_t last_frame_events;    /**< Events in the last completed batch. */
        uint8_t lane;                 /**< Next lane in the rotation. */
        uint8_t fault;                /**< Latched driver-contract fault. */
        uint8_t initialized;          /**< Nonzero after KymoMux_Init. */
    } KymoMux;

    /**
     * @brief One-frame hand-over slot from the telemetry runtime into the mux.
     * @details Its functions match KymoWriteFn, KymoBusyFn and KymoMuxNextDataFn.
     */
    typedef struct KymoMuxDataSlot
    {
        uint8_t frame[KYMO_FRAME_MAX_SIZE]; /**< Copied DATA frame. */
        uint8_t length;                     /**< Frame length. */
        uint8_t pending;                    /**< Nonzero while the mux has not taken the frame. */
        uint8_t initialized;                /**< Nonzero after KymoMuxDataSlot_Init. */
    } KymoMuxDataSlot;

    /**
     * @brief Validate and copy the configuration into a persistent mux.
     * @param[out] mux Caller-owned mux storage.
     * @param[in] config Configuration; may be temporary.
     * @return KYMO_OK, or KYMO_BAD_CONFIG for missing recorder/write or invalid sizes.
     */
    KymoStatus KymoMux_Init(KymoMux *mux, const KymoMuxConfig *config);

    /**
     * @brief Queue an administrative record; the body is copied and protected by CRC-32.
     * @param[in,out] mux Initialized mux.
     * @param[in] kind SESSION_META, DICTIONARY, HEALTH or SESSION_END.
     * @param[in] generation Record generation.
     * @param[in] body Record body.
     * @param[in] body_length Body length, at most KYMO_MUX_ADMIN_RECORD_MAX - 4.
     * @return KYMO_OK, KYMO_BUSY while a record is still queued, or KYMO_BAD_CONFIG.
     */
    KymoStatus KymoMux_QueueAdmin(KymoMux *mux, uint8_t kind, uint32_t generation,
                                  const uint8_t *body, size_t body_length);

    /**
     * @brief Queue SESSION_END after producer quiescence and ring settlement.
     * @details The application must stop all producers first. The settled frontier is
     * exclusive: one past the last accounted record sequence, including drops.
     * @param[in,out] mux Initialized mux.
     * @param[in] generation Session generation to include in the record.
     * @return KYMO_OK when queued; KYMO_BUSY while frames, records or events are still
     *         pending (service the mux and retry); KYMO_BAD_CONFIG for an invalid mux.
     */
    KymoStatus KymoMux_QueueSessionEnd(KymoMux *mux, uint32_t generation);

    /**
     * @brief Service at most one whole-frame attempt.
     * @param[in,out] mux Initialized mux.
     * @return KYMO_OK when a frame completed, KYMO_IDLE when nothing is queued,
     *         KYMO_BUSY while a frame is pending or the transport is busy,
     *         KYMO_IO_ERROR after a driver fault, KYMO_BAD_CONFIG for an invalid mux.
     */
    KymoStatus KymoMux_Service(KymoMux *mux);

    /** @brief Service only queued administrative records, holding event/DATA lanes.
     * @details For startup ordering before event batches. Uses the same partial-write
     * and borrowed-buffer contract as Service. Never mix this with a pending non-admin
     * frame: that request is rejected without discarding bytes. No periodic health is
     * produced here. Resume normal Service after the required startup records finish.
     * @param[in,out] mux Initialized mux.
     * @return OK, IDLE, BUSY, IO_ERROR, BAD_CONFIG or REJECTED for a pending other lane.
     * @par Usage Send metadata and dictionary before previously recorded events.
     */
    KymoStatus KymoMux_ServiceAdmin(KymoMux *mux);

    /**
     * @brief Initialize an empty data slot.
     * @param[out] slot Caller-owned slot.
     * @return KYMO_OK, or KYMO_BAD_CONFIG for NULL.
     */
    KymoStatus KymoMuxDataSlot_Init(KymoMuxDataSlot *slot);

    /**
     * @brief KymoWriteFn: accept one complete DATA frame into the slot.
     * @param[in,out] slot KymoMuxDataSlot pointer.
     * @param[in] bytes Complete frame.
     * @param[in] length Frame length.
     * @return length when copied; zero while occupied or for an invalid frame.
     */
    uint8_t KymoMuxDataSlot_Write(void *slot, const uint8_t *bytes, uint8_t length);

    /**
     * @brief KymoBusyFn: report the slot occupied.
     * @param[in,out] slot KymoMuxDataSlot pointer.
     * @return Nonzero while a frame waits for the mux.
     */
    uint8_t KymoMuxDataSlot_IsBusy(void *slot);

    /**
     * @brief KymoMuxNextDataFn: hand the queued frame to the mux and free the slot.
     * @param[in,out] slot KymoMuxDataSlot pointer.
     * @param[out] output Destination buffer.
     * @param[in] capacity Destination capacity.
     * @return Frame length, or zero when empty or capacity is too small.
     */
    size_t KymoMuxDataSlot_NextData(void *slot, uint8_t *output, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif /* KYMO_EFFECTIVE_TRACE */
#endif /* KYMO_MUX_H */
