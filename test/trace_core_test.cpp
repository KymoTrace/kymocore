/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "trace/kymo_trace.h"
static uint64_t ticks;
static uint8_t core;
static uint32_t context;
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
static void critical(void *)
{
}
int main(void)
{
    KymoTraceConfig config;
    memset(&config, 0, sizeof(config));
    config.clock_ticks = clockRead;
    config.core_id = coreRead;
    config.current_context = contextRead;
    config.critical_enter = critical;
    config.critical_exit = critical;
    config.session_id = 0x0102030405060708ULL;
    config.clock_domain_id = 1u;
    KymoTraceRecorder recorder;
    assert(KymoTrace_Init(&recorder, &config) == KYMO_OK);
    KymoTrace_Enter(&recorder, 1u, 1u);
    KymoTrace_Enter(&recorder, 1u, 2u);
    KymoTrace_Exit(&recorder, 1u, 2u);
    KymoTrace_IsrEnter(&recorder, 9u);
    assert(recorder.counts[0] == 4u);
    KymoTraceEvent event;
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK);
    assert(event.event_type == KYMO_TRACE_EVENT_ENTER && event.depth == 1u);
    assert((event.flags & KYMO_TRACE_FLAG_ROOT_BOUNDARY) != 0u);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK && event.depth == 2u);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK && event.depth == 2u);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK && event.arg == context);
    for (uint16_t i = 0; i < KYMO_TRACE_EVENTS_PER_CORE + 1u; ++i)
    {
        KymoTrace_Mark(&recorder, 1u, 4u, i);
    }
    assert(KymoTrace_DropCount(&recorder, 0u) == 1u);
    for (uint32_t i = 0u; i < KYMO_TRACE_MAX_DEPTH + 1u; ++i)
    {
        KymoTrace_Enter(&recorder, 2u, 1u);
    }
    for (uint32_t i = 0u; i < KYMO_TRACE_MAX_DEPTH + 1u; ++i)
    {
        KymoTrace_Exit(&recorder, 2u, 1u);
    }
    uint32_t sequenceBeforeReset = recorder.contexts[1].sequence;
    assert(KymoTrace_ContextReset(&recorder, 2u, 7u) == KYMO_OK);
    while (KymoTrace_Pop(&recorder, &event) == KYMO_OK)
    {
        if (event.event_type == KYMO_TRACE_EVENT_CONTEXT_RESET)
        {
            assert(event.context_seq >= sequenceBeforeReset);
        }
    }
    while (KymoTrace_Pop(&recorder, &event) == KYMO_OK)
    {
    }
    KymoTrace_Scheduler(&recorder, 1u, 2u);
    KymoTrace_TaskCreate(&recorder, 2u, 1u);
    KymoTrace_TaskEnd(&recorder, 2u, 1u);
    KymoTrace_CoreState(&recorder, 2u, 1u, 1u);
    KymoTrace_FlowSend(&recorder, 1u, 3u, 99u);
    KymoTrace_FlowReceive(&recorder, 2u, 3u, 99u);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK &&
           event.event_type == KYMO_TRACE_EVENT_TASK_SWITCH && event.arg == 2u);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK &&
           event.event_type == KYMO_TRACE_EVENT_TASK_CREATE);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK &&
           event.event_type == KYMO_TRACE_EVENT_TASK_END);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK &&
           event.event_type == KYMO_TRACE_EVENT_CORE_STATE &&
           (event.flags & KYMO_TRACE_FLAG_RECOVERY) != 0u);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK &&
           event.event_type == KYMO_TRACE_EVENT_FLOW_SEND);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK &&
           event.event_type == KYMO_TRACE_EVENT_FLOW_RECEIVE);
    uint8_t payload[40];
    KymoTraceEvent golden;
    memset(&golden, 0, sizeof(golden));
    golden.timestamp_ticks = 100u;
    golden.record_seq = 1u;
    golden.context_id = 1u;
    golden.context_seq = 1u;
    golden.symbol_id = 1u;
    golden.depth = 1u;
    golden.event_type = KYMO_TRACE_EVENT_ENTER;
    golden.flags = KYMO_TRACE_FLAG_ROOT_BOUNDARY;
    assert(KymoTrace_EncodeBatch(1u, &golden, 1u, payload, sizeof(payload)) == 36u);
    assert(payload[0] == 1u && payload[1] == 1u && payload[4] == 100u);
    KymoTraceEnvelopeInfo info = {0x0102030405060708ULL, 7u, KYMO_TRACE_PAYLOAD_BATCH};
    uint8_t frame[64];
    size_t length = KymoTrace_EncodeEnvelope(&info, payload, 36u, frame, sizeof(frame));
    assert(length == 60u && frame[0] == 0xa5u && frame[1] == 0x5au);
    assert(KymoTrace_Crc16((const uint8_t *)"123456789", 9u) == 0x29b1u);
    return 0;
}
