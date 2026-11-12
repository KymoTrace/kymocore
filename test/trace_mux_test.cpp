/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "transport/kymo_mux.h"
static uint64_t ticks;
static uint8_t core;
static uint32_t context;
static uint8_t busy;
static uint8_t holdNext;
static uint16_t frame_count;
static uint8_t frames[256][KYMO_TRACE_FRAME_MAX];
static uint16_t lengths[256];
static uint8_t dataRemaining = 8u;
static uint64_t clockRead(void *)
{
    return ticks++;
}
static uint8_t coreRead(void *)
{
    return core;
}
static uint32_t contextRead(void *)
{
    return context;
}
static uint8_t isBusy(void *)
{
    return busy;
}
static size_t writeFrame(void *, const uint8_t *bytes, size_t length)
{
    size_t result = 0u;
    if (busy == 0u)
    {
        assert(length <= KYMO_TRACE_FRAME_MAX);
        memcpy(frames[frame_count], bytes, length);
        lengths[frame_count] = static_cast<uint16_t>(length);
        ++frame_count;
        if (holdNext != 0u)
        {
            holdNext = 0u;
            busy = 1u;
        }
        result = length;
    }
    return result;
}
static size_t nextData(void *, uint8_t *out, size_t cap)
{
    size_t result = 0u;
    if ((cap >= 9u) && (dataRemaining != 0u))
    {
        --dataRemaining;
        out[0] = 0xa5u;
        out[1] = 0x5au;
        out[2] = 0x40u;
        out[3] = 7u;
        out[4] = 0u;
        out[5] = 0u;
        out[6] = 0x80u;
        out[7] = 0x3fu;
        out[8] = 0u;
        result = 9u;
    }
    return result;
}
int main(void)
{
    KymoTraceConfig recorderConfig;
    memset(&recorderConfig, 0, sizeof(recorderConfig));
    recorderConfig.clock_ticks = clockRead;
    recorderConfig.core_id = coreRead;
    recorderConfig.current_context = contextRead;
    recorderConfig.session_id = 44u;
    KymoTraceRecorder recorder;
    assert(KymoTrace_Init(&recorder, &recorderConfig) == KYMO_OK);
    KymoMuxConfig muxConfig;
    memset(&muxConfig, 0, sizeof(muxConfig));
    muxConfig.recorder = &recorder;
    muxConfig.write = writeFrame;
    muxConfig.busy = isBusy;
    muxConfig.next_data = nextData;
    muxConfig.max_frame_size = 64u;
    muxConfig.health_period_ticks = 100u;
    KymoMux mux;
    assert(KymoMux_Init(&mux, &muxConfig) == KYMO_OK);
    uint8_t metadata[90];
    for (uint8_t i = 0; i < sizeof(metadata); ++i)
    {
        metadata[i] = i;
    }
    assert(KymoMux_QueueAdmin(&mux, KYMO_TRACE_PAYLOAD_SESSION_META, 3u, metadata,
                              sizeof(metadata)) == KYMO_OK);
    uint16_t prior = frame_count;
    for (uint16_t i = 0; i < 10u; ++i)
    {
        (void)KymoMux_Service(&mux);
    }
    assert(frame_count > prior);
    uint32_t expectedOffset = 0u;
    uint8_t metaFrames = 0u;
    for (uint16_t i = prior; i < frame_count; ++i)
    {
        assert(lengths[i] <= 64u);
        if (frames[i][3] == KYMO_TRACE_PAYLOAD_SESSION_META)
        {
            assert(frames[i][4] <= 40u);
            uint32_t offset =
                (uint32_t)frames[i][22u + 14u] | ((uint32_t)frames[i][22u + 15u] << 8) |
                ((uint32_t)frames[i][22u + 16u] << 16) | ((uint32_t)frames[i][22u + 17u] << 24);
            assert(offset == expectedOffset);
            size_t fragment = (size_t)frames[i][4u] - 18u;
            expectedOffset += (uint32_t)fragment;
            ++metaFrames;
        }
    }
    assert(metaFrames == 5u && expectedOffset == 94u);
    for (uint16_t i = 0; i < KYMO_TRACE_EVENTS_PER_CORE + 6u; ++i)
    {
        KymoTrace_Mark(&recorder, 1u, 2u, i);
    }
    assert(KymoTrace_DropCount(&recorder, 0u) == 6u);
    prior = frame_count;
    for (uint16_t i = 0; i < 400u; ++i)
    {
        (void)KymoMux_Service(&mux);
    }
    uint8_t foundData = 0u;
    uint8_t foundHealth = 0u;
    uint8_t healthRecord[128];
    memset(healthRecord, 0, sizeof(healthRecord));
    uint32_t healthTotal = 0u;
    uint32_t healthReceived = 0u;
    uint32_t healthTransfer = 0u;
    for (uint16_t i = prior; i < frame_count; ++i)
    {
        uint8_t kind = frames[i][3];
        if (kind == KYMO_TRACE_PAYLOAD_DATA_V1)
        {
            foundData = 1u;
        }
        if (kind == KYMO_TRACE_PAYLOAD_HEALTH)
        {
            foundHealth = 1u;
            uint32_t transfer = (uint32_t)frames[i][28] | ((uint32_t)frames[i][29] << 8) |
                                ((uint32_t)frames[i][30] << 16) | ((uint32_t)frames[i][31] << 24);
            if (healthTransfer == 0u)
            {
                healthTransfer = transfer;
            }
            if (transfer == healthTransfer)
            {
                uint32_t offset = (uint32_t)frames[i][36] | ((uint32_t)frames[i][37] << 8) |
                                  ((uint32_t)frames[i][38] << 16) | ((uint32_t)frames[i][39] << 24);
                uint32_t total = (uint32_t)frames[i][32] | ((uint32_t)frames[i][33] << 8) |
                                 ((uint32_t)frames[i][34] << 16) | ((uint32_t)frames[i][35] << 24);
                uint16_t payloadLength = (uint16_t)(frames[i][4] | ((uint16_t)frames[i][5] << 8));
                uint32_t fragmentLength = (uint32_t)payloadLength - 18u;
                if (offset + fragmentLength <= sizeof(healthRecord))
                {
                    memcpy(healthRecord + offset, frames[i] + 40u, fragmentLength);
                    healthTotal = total;
                    healthReceived += fragmentLength;
                }
            }
        }
        assert(lengths[i] <= 64u);
    }
    assert(foundData != 0u);
    assert(foundHealth != 0u);
    assert(healthTotal == 40u);
    assert(healthReceived == healthTotal);
    uint32_t reportedDrops = (uint32_t)healthRecord[20] | ((uint32_t)healthRecord[21] << 8) |
                             ((uint32_t)healthRecord[22] << 16) |
                             ((uint32_t)healthRecord[23] << 24);
    assert(reportedDrops == 6u);
    KymoMuxDataSlot slot;
    assert(KymoMuxDataSlot_Init(&slot) == KYMO_OK);
    uint8_t dataFrame[9] = {0xa5u, 0x5au, 0x40u, 7u, 0u, 0u, 0x80u, 0x3fu, 0u};
    assert(KymoMuxDataSlot_Write(&slot, dataFrame, 9u) == 9u);
    assert(KymoMuxDataSlot_IsBusy(&slot) == 1u);
    assert(KymoMuxDataSlot_Write(&slot, dataFrame, 9u) == 0u);
    uint8_t copy[21];
    assert(KymoMuxDataSlot_NextData(&slot, copy, 8u) == 0u);
    assert(KymoMuxDataSlot_NextData(&slot, copy, sizeof(copy)) == 9u);
    assert(memcmp(copy, dataFrame, 9u) == 0);
    assert(KymoMuxDataSlot_IsBusy(&slot) == 0u);
    uint8_t micro[13] = {0xa5u, 0x5au, 0x53u, 3u,    0u, 0u, 0x20u,
                         0xc0u, 0x87u, 0xd6u, 0x12u, 0u, 0u};
    assert(KymoMuxDataSlot_Write(&slot, micro, 13u) == 13u);
    assert(KymoMuxDataSlot_NextData(&slot, copy, sizeof(copy)) == 13u);
    uint8_t unitOnly[9] = {0xa5u, 0x5au, 0x51u, 7u, 0u, 0u, 0x80u, 0x3fu, 0u};
    assert(KymoMuxDataSlot_Write(&slot, unitOnly, 9u) == 0u);
    holdNext = 1u;
    KymoTrace_Mark(&recorder, 1u, 2u, 99u);
    for (uint8_t i = 0; i < 4u; ++i)
    {
        (void)KymoMux_Service(&mux);
    }
    assert(mux.pending != 0u);
    uint8_t saved[KYMO_TRACE_FRAME_MAX];
    size_t savedLength = mux.frame_length;
    memcpy(saved, mux.frame, savedLength);
    busy = 0u;
    (void)KymoMux_Service(&mux);
    assert(mux.pending == 0u);
    assert(memcmp(saved, frames[frame_count - 1u], savedLength) == 0);
    KymoTraceRecorder largeRecorder;
    assert(KymoTrace_Init(&largeRecorder, &recorderConfig) == KYMO_OK);
    KymoMuxConfig largeConfig = muxConfig;
    largeConfig.recorder = &largeRecorder;
    largeConfig.next_data = 0;
    largeConfig.max_frame_size = 240u;
    largeConfig.health_period_ticks = 0u;
    KymoMux largeMux;
    assert(KymoMux_Init(&largeMux, &largeConfig) == KYMO_OK);
    for (uint8_t i = 0; i < 6u; ++i)
    {
        KymoTrace_Mark(&largeRecorder, 1u, 2u, i);
    }
    uint16_t batchStart = frame_count;
    (void)KymoMux_Service(&largeMux);
    assert(frame_count == batchStart + 1u);
    assert(lengths[batchStart] == 220u);
    assert(frames[batchStart][3] == KYMO_TRACE_PAYLOAD_BATCH);
    assert(frames[batchStart][23] == 6u);
    assert(KymoMux_QueueSessionEnd(&mux, 7u) == KYMO_OK);
    uint16_t endStart = frame_count;
    for (uint8_t serviceIndex = 0u; serviceIndex < 8u; ++serviceIndex)
    {
        (void)KymoMux_Service(&mux);
    }
    uint8_t foundSessionEnd = 0u;
    uint8_t sessionEndRecord[64];
    memset(sessionEndRecord, 0, sizeof(sessionEndRecord));
    uint32_t sessionEndReceived = 0u;
    for (uint16_t frameIndex = endStart; frameIndex < frame_count; ++frameIndex)
    {
        if (frames[frameIndex][3] == KYMO_TRACE_PAYLOAD_SESSION_END)
        {
            uint32_t fragmentOffset =
                (uint32_t)frames[frameIndex][36] | ((uint32_t)frames[frameIndex][37] << 8) |
                ((uint32_t)frames[frameIndex][38] << 16) | ((uint32_t)frames[frameIndex][39] << 24);
            uint16_t payloadLength =
                (uint16_t)frames[frameIndex][4] | ((uint16_t)frames[frameIndex][5] << 8);
            uint32_t fragmentLength = (uint32_t)payloadLength - 18u;
            assert(fragmentOffset + fragmentLength <= sizeof(sessionEndRecord));
            memcpy(sessionEndRecord + fragmentOffset, frames[frameIndex] + 40u, fragmentLength);
            sessionEndReceived += fragmentLength;
            foundSessionEnd = 1u;
        }
    }
    assert(foundSessionEnd != 0u);
    assert(sessionEndReceived == 32u);
    uint32_t producerNext = (uint32_t)sessionEndRecord[8] | ((uint32_t)sessionEndRecord[9] << 8) |
                            ((uint32_t)sessionEndRecord[10] << 16) |
                            ((uint32_t)sessionEndRecord[11] << 24);
    uint32_t settledThrough =
        (uint32_t)sessionEndRecord[12] | ((uint32_t)sessionEndRecord[13] << 8) |
        ((uint32_t)sessionEndRecord[14] << 16) | ((uint32_t)sessionEndRecord[15] << 24);
    assert(producerNext == settledThrough);
    return 0;
}
