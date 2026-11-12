/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */

/**
 * @file kymo_trace_protocol.h
 * @brief Trace wire protocol v2: constants, event layout and fieldwise encoders.
 * @details No target layout is copied to the wire; every field is serialized
 * little-endian. This header has no transport API; the mux (kymo_mux.h) uses it to
 * build frames. Available with KYMO_ENABLE_TRACE=1.
 * @par Usage
 * Applications normally use the recorder and mux; encoders serve tests and host tools.
 */
#ifndef KYMO_TRACE_PROTOCOL_H
#define KYMO_TRACE_PROTOCOL_H

#include "../kymo_types.h"

#if KYMO_EFFECTIVE_TRACE || defined(DOXYGEN)

/** @brief Largest trace transport frame in bytes, including the envelope. */
#define KYMO_TRACE_FRAME_MAX 240u
/** @brief Envelope overhead in bytes (header plus trailing CRC). */
#define KYMO_TRACE_ENVELOPE_SIZE 24u
/** @brief Serialized size of one KymoTraceEvent in bytes. */
#define KYMO_TRACE_EVENT_SIZE 32u
/** @brief Common prefix of an administrative fragment in bytes. */
#define KYMO_TRACE_ADMIN_PREFIX_SIZE 18u

/** @brief Payload kind values of the v2 envelope. */
typedef enum KymoTracePayloadKind
{
    KYMO_TRACE_PAYLOAD_SESSION_META = 1, /**< Session metadata record. */
    KYMO_TRACE_PAYLOAD_DICTIONARY = 2,   /**< Symbol dictionary record. */
    KYMO_TRACE_PAYLOAD_BATCH = 3,        /**< Batch of trace events. */
    KYMO_TRACE_PAYLOAD_DATA_V1 = 4,      /**< Embedded telemetry DATA frame. */
    KYMO_TRACE_PAYLOAD_HEALTH = 5,       /**< Recorder health record. */
    KYMO_TRACE_PAYLOAD_SESSION_END = 6   /**< Final settled-frontier record. */
} KymoTracePayloadKind;

/** @brief Event type values of event schema v1. */
typedef enum KymoTraceEventType
{
    KYMO_TRACE_EVENT_ENTER = 1,          /**< Scope entry. */
    KYMO_TRACE_EVENT_EXIT = 2,           /**< Scope exit. */
    KYMO_TRACE_EVENT_MARK = 3,           /**< Marker with value. */
    KYMO_TRACE_EVENT_ISR_ENTER = 4,      /**< Interrupt entry. */
    KYMO_TRACE_EVENT_ISR_EXIT = 5,       /**< Interrupt exit. */
    KYMO_TRACE_EVENT_CONTEXT_RESET = 6,  /**< Context recovery. */
    KYMO_TRACE_EVENT_TASK_CREATE = 0x20, /**< Task creation. */
    KYMO_TRACE_EVENT_TASK_END = 0x21,    /**< Task termination. */
    KYMO_TRACE_EVENT_TASK_SWITCH = 0x22, /**< Task switch. */
    KYMO_TRACE_EVENT_CORE_STATE = 0x23,  /**< Per-core running-context checkpoint. */
    KYMO_TRACE_EVENT_FLOW_SEND = 0x30,   /**< Flow correlation send. */
    KYMO_TRACE_EVENT_FLOW_RECEIVE = 0x31 /**< Flow correlation receive. */
} KymoTraceEventType;

/** @brief One logical trace event; serialized fieldwise into 32 bytes. */
typedef struct KymoTraceEvent
{
    uint64_t timestamp_ticks; /**< Recorder clock reading. */
    uint32_t record_seq;      /**< Per-core record sequence number. */
    uint32_t context_id;      /**< Application context (task/ISR) ID. */
    uint32_t context_seq;     /**< Per-context sequence number. */
    uint16_t symbol_id;       /**< Function, scope or marker symbol ID. */
    uint16_t depth;           /**< Scope depth after the event. */
    uint8_t event_type;       /**< KymoTraceEventType value. */
    uint8_t core_id;          /**< Producing core. */
    uint16_t flags;           /**< KYMO_TRACE_FLAG_* bits. */
    uint32_t arg;             /**< Event-specific argument. */
} KymoTraceEvent;

/** @brief Envelope identifiers supplied by the caller. */
typedef struct KymoTraceEnvelopeInfo
{
    uint64_t session_id;  /**< Session identifier. */
    uint32_t packet_seq;  /**< Envelope sequence number. */
    uint8_t payload_kind; /**< KymoTracePayloadKind value. */
} KymoTraceEnvelopeInfo;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Compute CRC-16/CCITT-FALSE as used by the trace envelope.
     * @param[in] data Bytes, or NULL for empty input.
     * @param[in] length Byte count.
     * @return CRC value.
     */
    uint16_t KymoTrace_Crc16(const uint8_t *data, size_t length);

    /**
     * @brief Wrap a payload into a v2 envelope.
     * @param[in] info Envelope identifiers.
     * @param[in] payload Payload bytes, may be NULL when payload_length is zero.
     * @param[in] payload_length Payload size, at most KYMO_TRACE_FRAME_MAX - KYMO_TRACE_ENVELOPE_SIZE.
     * @param[out] output Destination buffer.
     * @param[in] capacity Destination capacity.
     * @return Encoded byte count, or zero on invalid bounds.
     */
    size_t KymoTrace_EncodeEnvelope(const KymoTraceEnvelopeInfo *info, const uint8_t *payload,
                                    size_t payload_length, uint8_t *output, size_t capacity);

    /**
     * @brief Encode a batch of events into a BATCH payload.
     * @param[in] clock_domain_id Clock domain of the timestamps.
     * @param[in] events Event array, may be NULL when event_count is zero.
     * @param[in] event_count Number of events, at most 255.
     * @param[out] output Destination buffer.
     * @param[in] capacity Destination capacity.
     * @return Encoded payload length, or zero on invalid bounds.
     */
    size_t KymoTrace_EncodeBatch(uint16_t clock_domain_id, const KymoTraceEvent *events,
                                 size_t event_count, uint8_t *output, size_t capacity);

    /**
     * @brief Encode one fragment of an administrative record.
     * @param[in] kind Payload kind of the record.
     * @param[in] generation Record generation.
     * @param[in] transfer_id Transfer identifier shared by all fragments.
     * @param[in] total_length Complete record length.
     * @param[in] offset Fragment offset inside the record.
     * @param[in] record Complete record bytes.
     * @param[in] fragment_length Bytes of this fragment.
     * @param[out] output Destination buffer.
     * @param[in] capacity Destination capacity.
     * @return Encoded fragment size, or zero on invalid bounds.
     */
    size_t KymoTrace_EncodeAdminFragment(uint8_t kind, uint32_t generation, uint32_t transfer_id,
                                         uint32_t total_length, uint32_t offset,
                                         const uint8_t *record, size_t fragment_length,
                                         uint8_t *output, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif /* KYMO_EFFECTIVE_TRACE */
#endif /* KYMO_TRACE_PROTOCOL_H */
