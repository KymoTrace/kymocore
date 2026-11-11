/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_mux.cpp
 * @brief Bounded trace, telemetry DATA and admin-record transport scheduler.
 */
#include "kymo_mux.h"

#if KYMO_EFFECTIVE_TRACE
#include "../common/embedded_bytes.h"
#include "../common/embedded_crc.h"
#include <string.h>

namespace {

/** @brief Lane identifiers stored in KymoMux::pending_kind. */
enum MuxLane {
    LANE_NONE = 0u,
    LANE_ADMIN = 1u,
    LANE_TRACE = 2u,
    LANE_DATA = 3u
};

/** @brief Most events a single batch frame can carry. */
const uint8_t MAX_BATCH_EVENTS = 6u;

/*******************************************************************************
 * transportBusy
 ******************************************************************************/
uint8_t transportBusy(const KymoMux *mux)
{
    uint8_t busy = 0u;
    if (mux->config.busy != NULL) {
        busy = (mux->config.busy(mux->config.transport_user) != 0u) ? 1u : 0u;
    }
    return busy;
}

/*******************************************************************************
 * recorderLock
 ******************************************************************************/
void recorderLock(KymoTraceRecorder *recorder)
{
    if (recorder->config.critical_enter != NULL) {
        recorder->config.critical_enter(recorder->config.user);
    }
}

/*******************************************************************************
 * recorderUnlock
 ******************************************************************************/
void recorderUnlock(KymoTraceRecorder *recorder)
{
    if (recorder->config.critical_exit != NULL) {
        recorder->config.critical_exit(recorder->config.user);
    }
}

/*******************************************************************************
 * allRingsEmpty
 ******************************************************************************/
uint8_t allRingsEmpty(const KymoTraceRecorder *recorder)
{
    uint8_t empty = 1u;
    for (uint8_t core = 0u; core < KYMO_TRACE_CORE_COUNT; ++core) {
        if (recorder->counts[core] != 0u) {
            empty = 0u;
        }
    }
    return empty;
}

/*******************************************************************************
 * validDataFrame
 ******************************************************************************/
uint8_t validDataFrame(const uint8_t *frame, size_t length)
{
    uint8_t valid = 0u;
    if ((frame != NULL) && (length >= KYMO_FRAME_MIN_SIZE) &&
        (length <= KYMO_FRAME_MAX_SIZE) && (frame[0] == KYMO_SYNC_0) &&
        (frame[1] == KYMO_SYNC_1)) {
        uint8_t descriptor = frame[2];
        uint8_t flags = (uint8_t)(descriptor & KYMO_ALLOWED_FLAGS);
        size_t expected = KYMO_FRAME_MIN_SIZE;
        if (((descriptor & KYMO_DESCRIPTOR_VERSION_MASK) ==
             KYMO_DESCRIPTOR_DATA) &&
            ((descriptor & KYMO_DESCRIPTOR_RESERVED_MASK) == 0u) &&
            (((descriptor & KYMO_FLAG_TIMESTAMP_US) == 0u) ||
             ((flags & KYMO_FLAG_TIMESTAMP) != 0u))) {
            if ((flags & KYMO_FLAG_X) != 0u) {
                expected += 4u;
            }
            if ((flags & KYMO_FLAG_Z) != 0u) {
                expected += 4u;
            }
            if ((flags & KYMO_FLAG_TIMESTAMP) != 0u) {
                expected += 4u;
            }
            if (length == expected) {
                valid = 1u;
            }
        }
    }
    return valid;
}

/*******************************************************************************
 * adminRequestValid
 ******************************************************************************/
uint8_t adminRequestValid(uint8_t kind, const uint8_t *body, size_t bodyLength)
{
    uint8_t valid = (uint8_t)((body != NULL) &&
        (bodyLength <= KYMO_MUX_ADMIN_RECORD_MAX - 4u) &&
        (kind >= KYMO_TRACE_PAYLOAD_SESSION_META) &&
        (kind <= KYMO_TRACE_PAYLOAD_SESSION_END) &&
        (kind != KYMO_TRACE_PAYLOAD_BATCH) &&
        (kind != KYMO_TRACE_PAYLOAD_DATA_V1));
    return valid;
}

/*******************************************************************************
 * prepareAdmin
 ******************************************************************************/
uint8_t prepareAdmin(KymoMux *mux)
{
    uint8_t result = 0u;
    size_t fragmentCapacity = (size_t)mux->config.max_frame_size -
        KYMO_TRACE_ENVELOPE_SIZE - KYMO_TRACE_ADMIN_PREFIX_SIZE;
    size_t remaining = (size_t)(mux->admin_total - mux->admin_offset);
    mux->pending_admin_bytes = (uint16_t)((remaining < fragmentCapacity) ?
        remaining : fragmentCapacity);
    uint8_t payload[KYMO_TRACE_FRAME_MAX - KYMO_TRACE_ENVELOPE_SIZE];
    size_t payloadLength = KymoTrace_EncodeAdminFragment(mux->admin_kind,
        mux->admin_generation, mux->transfer_id, mux->admin_total,
        mux->admin_offset, mux->admin_record, mux->pending_admin_bytes,
        payload, sizeof(payload));
    if (payloadLength > 0u) {
        KymoTraceEnvelopeInfo info = {mux->config.recorder->config.session_id,
            mux->packet_seq++, mux->admin_kind};
        mux->frame_length = KymoTrace_EncodeEnvelope(&info, payload,
            payloadLength, mux->frame, sizeof(mux->frame));
        mux->pending_kind = LANE_ADMIN;
        result = (mux->frame_length > 0u) ? 1u : 0u;
    }
    return result;
}

/*******************************************************************************
 * prepareTrace
 ******************************************************************************/
uint8_t prepareTrace(KymoMux *mux)
{
    uint8_t result = 0u;
    KymoTraceEvent events[MAX_BATCH_EVENTS];
    uint8_t payload[KYMO_TRACE_FRAME_MAX - KYMO_TRACE_ENVELOPE_SIZE];
    size_t budget = (mux->config.max_frame_size - KYMO_TRACE_ENVELOPE_SIZE - 4u) /
        KYMO_TRACE_EVENT_SIZE;
    size_t limit = mux->config.max_batch_events;
    if (limit > budget) {
        limit = budget;
    }
    size_t count = 0u;
    while ((count < limit) &&
           (KymoTrace_Pop(mux->config.recorder, &events[count]) == KYMO_OK)) {
        ++count;
    }
    if (count > 0u) {
        size_t payloadLength = KymoTrace_EncodeBatch(
            mux->config.recorder->config.clock_domain_id, events, count,
            payload, sizeof(payload));
        if (payloadLength > 0u) {
            KymoTraceEnvelopeInfo info = {mux->config.recorder->config.session_id,
                mux->packet_seq++, KYMO_TRACE_PAYLOAD_BATCH};
            mux->frame_length = KymoTrace_EncodeEnvelope(&info, payload,
                payloadLength, mux->frame, sizeof(mux->frame));
            mux->pending_kind = LANE_TRACE;
            result = (mux->frame_length > 0u) ? 1u : 0u;
        }
    }
    return result;
}

/*******************************************************************************
 * prepareData
 ******************************************************************************/
uint8_t prepareData(KymoMux *mux)
{
    uint8_t result = 0u;
    if (mux->config.next_data != NULL) {
        uint8_t data[KYMO_TRACE_FRAME_MAX];
        size_t capacity = (size_t)mux->config.max_frame_size -
            KYMO_TRACE_ENVELOPE_SIZE;
        size_t dataLength = mux->config.next_data(mux->config.data_user, data,
                                                  capacity);
        if ((dataLength <= capacity) &&
            (validDataFrame(data, dataLength) != 0u)) {
            KymoTraceEnvelopeInfo info = {mux->config.recorder->config.session_id,
                mux->packet_seq++, KYMO_TRACE_PAYLOAD_DATA_V1};
            mux->frame_length = KymoTrace_EncodeEnvelope(&info, data,
                dataLength, mux->frame, sizeof(mux->frame));
            mux->pending_kind = LANE_DATA;
            result = (mux->frame_length > 0u) ? 1u : 0u;
        }
    }
    return result;
}

/*******************************************************************************
 * finishAccepted
 ******************************************************************************/
void finishAccepted(KymoMux *mux)
{
    mux->last_frame_length = mux->frame_length;
    mux->last_frame_events = (mux->pending_kind == LANE_TRACE) ? mux->frame[23u] : 0u;
    if (mux->pending_kind == LANE_ADMIN) {
        mux->admin_offset += mux->pending_admin_bytes;
        if (mux->admin_offset >= mux->admin_total) {
            mux->admin_pending = 0u;
        }
    }
    mux->pending = 0u;
    mux->pending_kind = LANE_NONE;
    mux->frame_length = 0u;
    mux->frame_offset = 0u;
}

/*******************************************************************************
 * writePending
 ******************************************************************************/
KymoStatus writePending(KymoMux *mux)
{
    KymoStatus result = KYMO_BUSY;
    if (mux->frame_offset < mux->frame_length) {
        size_t remaining = mux->frame_length - mux->frame_offset;
        size_t accepted = mux->config.write(mux->config.transport_user,
            mux->frame + mux->frame_offset, remaining);
        if (accepted > remaining) {
            mux->fault = 1u;
            result = KYMO_IO_ERROR;
        } else {
            mux->frame_offset += accepted;
        }
    }
    if ((result != KYMO_IO_ERROR) && (mux->frame_offset == mux->frame_length) &&
        (transportBusy(mux) == 0u)) {
        finishAccepted(mux);
        result = KYMO_OK;
    }
    return result;
}

/*******************************************************************************
 * writeHealthBody
 ******************************************************************************/
void writeHealthBody(KymoTraceRecorder *recorder, uint64_t now, uint8_t *body)
{
    body[0] = 1u;
    body[1] = KYMO_TRACE_CORE_COUNT;
    emb_write_u16_le(body + 2u, 0u);
    for (uint8_t core = 0u; core < KYMO_TRACE_CORE_COUNT; ++core) {
        uint8_t *entry = body + 4u + (size_t)core * 32u;
        recorder->settled_through[core] = recorder->producer_next[core];
        entry[0] = core;
        entry[1] = 1u;
        emb_write_u16_le(entry + 2u, 0u);
        emb_write_u16_le(entry + 4u, recorder->high_water[core]);
        emb_write_u16_le(entry + 6u, 0u);
        emb_write_u32_le(entry + 8u, recorder->producer_next[core]);
        emb_write_u32_le(entry + 12u, recorder->settled_through[core]);
        emb_write_u32_le(entry + 16u, recorder->drops[core]);
        emb_write_u32_le(entry + 20u, recorder->errors[core]);
        emb_write_u64_le(entry + 24u, now);
    }
}

/*******************************************************************************
 * prepareHealth
 ******************************************************************************/
void prepareHealth(KymoMux *mux)
{
    uint8_t shouldQueue = 0u;
    uint8_t body[4u + 32u * KYMO_TRACE_CORE_COUNT];
    KymoTraceRecorder *recorder = mux->config.recorder;
    if ((mux->config.health_period_ticks > 0u) &&
        (mux->admin_pending == 0u) && (mux->pending == 0u)) {
        recorderLock(recorder);
        uint64_t now = recorder->config.clock_ticks(recorder->config.user);
        if ((now >= mux->next_health_ticks) &&
            (allRingsEmpty(recorder) != 0u)) {
            writeHealthBody(recorder, now, body);
            mux->next_health_ticks = now + mux->config.health_period_ticks;
            shouldQueue = 1u;
        }
        recorderUnlock(recorder);
        if (shouldQueue != 0u) {
            (void)KymoMux_QueueAdmin(mux, KYMO_TRACE_PAYLOAD_HEALTH, 1u, body,
                                     sizeof(body));
        }
    }
}

/*******************************************************************************
 * startNext
 ******************************************************************************/
KymoStatus startNext(KymoMux *mux)
{
    KymoStatus result = KYMO_IDLE;
    uint8_t made = 0u;
    prepareHealth(mux);
    for (uint8_t attempt = 0u; (attempt < 3u) && (made == 0u); ++attempt) {
        uint8_t lane = (uint8_t)((mux->lane + attempt) % 3u);
        if ((lane == 0u) && (mux->admin_pending != 0u)) {
            made = prepareAdmin(mux);
        } else if (lane == 1u) {
            made = prepareTrace(mux);
        } else if (lane == 2u) {
            made = prepareData(mux);
        }
        if (made != 0u) {
            mux->pending = 1u;
            mux->frame_offset = 0u;
            mux->lane = (uint8_t)((lane + 1u) % 3u);
            result = writePending(mux);
        }
    }
    return result;
}

} // namespace

/*******************************************************************************
 * KymoMux_Init
 ******************************************************************************/
KymoStatus KymoMux_Init(KymoMux *mux, const KymoMuxConfig *config)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((mux != NULL) && (config != NULL) && (config->recorder != NULL) &&
        (config->recorder->initialized != 0u) &&
        (config->recorder->config.clock_ticks != NULL) &&
        (config->write != NULL) &&
        ((config->max_frame_size == 0u) ||
         ((config->max_frame_size >= 64u) &&
          (config->max_frame_size <= KYMO_TRACE_FRAME_MAX))) &&
        (config->max_batch_events <= MAX_BATCH_EVENTS)) {
        memset(mux, 0, sizeof(*mux));
        mux->config = *config;
        if (mux->config.max_frame_size == 0u) {
            mux->config.max_frame_size = KYMO_TRACE_FRAME_MAX;
        }
        if (mux->config.max_batch_events == 0u) {
            mux->config.max_batch_events = MAX_BATCH_EVENTS;
        }
        mux->next_health_ticks = config->recorder->config.clock_ticks(
            config->recorder->config.user) + config->health_period_ticks;
        mux->initialized = 1u;
        result = KYMO_OK;
    }
    return result;
}

/*******************************************************************************
 * KymoMux_QueueAdmin
 ******************************************************************************/
KymoStatus KymoMux_QueueAdmin(KymoMux *mux, uint8_t kind, uint32_t generation,
                              const uint8_t *body, size_t bodyLength)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((mux != NULL) && (mux->initialized != 0u) &&
        (adminRequestValid(kind, body, bodyLength) != 0u)) {
        if (mux->admin_pending != 0u) {
            result = KYMO_BUSY;
        } else {
            memcpy(mux->admin_record, body, bodyLength);
            emb_write_u32_le(mux->admin_record + bodyLength,
                             emb_crc32_ieee(body, bodyLength));
            mux->admin_total = (uint32_t)(bodyLength + 4u);
            mux->admin_offset = 0u;
            mux->admin_generation = generation;
            mux->admin_kind = kind;
            ++mux->transfer_id;
            mux->admin_pending = 1u;
            result = KYMO_OK;
        }
    }
    return result;
}

/*******************************************************************************
 * KymoMux_QueueSessionEnd
 ******************************************************************************/
KymoStatus KymoMux_QueueSessionEnd(KymoMux *mux, uint32_t generation)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    uint8_t body[4u + 24u * KYMO_TRACE_CORE_COUNT] = {};
    if ((mux != NULL) && (mux->initialized != 0u)) {
        result = KYMO_BUSY;
        if ((mux->pending == 0u) && (mux->admin_pending == 0u) &&
            (transportBusy(mux) == 0u)) {
            KymoTraceRecorder *recorder = mux->config.recorder;
            recorderLock(recorder);
            uint8_t empty = allRingsEmpty(recorder);
            if (empty != 0u) {
                body[0] = 1u;
                body[1] = KYMO_TRACE_CORE_COUNT;
                for (uint8_t core = 0u; core < KYMO_TRACE_CORE_COUNT; ++core) {
                    uint8_t *entry = body + 4u + (size_t)core * 24u;
                    recorder->settled_through[core] = recorder->producer_next[core];
                    entry[0] = core;
                    emb_write_u32_le(entry + 4u, recorder->producer_next[core]);
                    emb_write_u32_le(entry + 8u, recorder->settled_through[core]);
                    emb_write_u32_le(entry + 12u, recorder->drops[core]);
                    emb_write_u64_le(entry + 16u,
                        recorder->config.clock_ticks(recorder->config.user));
                }
            }
            recorderUnlock(recorder);
            if (empty != 0u) {
                result = KymoMux_QueueAdmin(mux, KYMO_TRACE_PAYLOAD_SESSION_END,
                                            generation, body, sizeof(body));
            }
        }
    }
    return result;
}

/*******************************************************************************
 * KymoMux_Service
 ******************************************************************************/
KymoStatus KymoMux_Service(KymoMux *mux)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((mux != NULL) && (mux->initialized != 0u)) {
        if (mux->fault != 0u) {
            result = KYMO_IO_ERROR;
        } else if (transportBusy(mux) != 0u) {
            result = KYMO_BUSY;
        } else if (mux->pending != 0u) {
            result = writePending(mux);
        } else {
            result = startNext(mux);
        }
    }
    return result;
}

/*******************************************************************************
 * KymoMux_ServiceAdmin
 ******************************************************************************/
KymoStatus KymoMux_ServiceAdmin(KymoMux *mux)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if ((mux != NULL) && (mux->initialized != 0u)) {
        if (mux->fault != 0u) {
            result = KYMO_IO_ERROR;
        } else if ((mux->pending != 0u) && (mux->pending_kind != LANE_ADMIN)) {
            result = KYMO_REJECTED;
        } else if (transportBusy(mux) != 0u) {
            result = KYMO_BUSY;
        } else if (mux->pending != 0u) {
            result = writePending(mux);
        } else if ((mux->admin_pending != 0u) && (prepareAdmin(mux) != 0u)) {
            mux->pending = 1u;
            mux->frame_offset = 0u;
            result = writePending(mux);
        } else {
            result = KYMO_IDLE;
        }
    }
    return result;
}

/*******************************************************************************
 * KymoMuxDataSlot_Init
 ******************************************************************************/
KymoStatus KymoMuxDataSlot_Init(KymoMuxDataSlot *slot)
{
    KymoStatus result = KYMO_BAD_CONFIG;
    if (slot != NULL) {
        memset(slot, 0, sizeof(*slot));
        slot->initialized = 1u;
        result = KYMO_OK;
    }
    return result;
}

/*******************************************************************************
 * KymoMuxDataSlot_Write
 ******************************************************************************/
uint8_t KymoMuxDataSlot_Write(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t result = 0u;
    KymoMuxDataSlot *slot = static_cast<KymoMuxDataSlot *>(user);
    if ((slot != NULL) && (slot->initialized != 0u) &&
        (slot->pending == 0u) && (validDataFrame(bytes, length) != 0u)) {
        memcpy(slot->frame, bytes, length);
        slot->length = length;
        slot->pending = 1u;
        result = length;
    }
    return result;
}

/*******************************************************************************
 * KymoMuxDataSlot_IsBusy
 ******************************************************************************/
uint8_t KymoMuxDataSlot_IsBusy(void *user)
{
    uint8_t result = 0u;
    KymoMuxDataSlot *slot = static_cast<KymoMuxDataSlot *>(user);
    if ((slot != NULL) && (slot->initialized != 0u)) {
        result = slot->pending;
    }
    return result;
}

/*******************************************************************************
 * KymoMuxDataSlot_NextData
 ******************************************************************************/
size_t KymoMuxDataSlot_NextData(void *user, uint8_t *output, size_t capacity)
{
    size_t result = 0u;
    KymoMuxDataSlot *slot = static_cast<KymoMuxDataSlot *>(user);
    if ((slot != NULL) && (output != NULL) &&
        (slot->initialized != 0u) && (slot->pending != 0u) &&
        (capacity >= slot->length)) {
        memcpy(output, slot->frame, slot->length);
        result = slot->length;
        slot->length = 0u;
        slot->pending = 0u;
    }
    return result;
}

#endif /* KYMO_EFFECTIVE_TRACE */
