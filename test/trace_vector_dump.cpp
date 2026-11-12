/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <string.h>
#include "trace/kymo_trace_protocol.h"
int main(void)
{
    KymoTraceEvent event;
    memset(&event, 0, sizeof(event));
    event.timestamp_ticks = 100u;
    event.record_seq = 1u;
    event.context_id = 1u;
    event.context_seq = 1u;
    event.symbol_id = 1u;
    event.depth = 1u;
    event.event_type = KYMO_TRACE_EVENT_ENTER;
    event.flags = 1u;
    uint8_t payload[36];
    uint8_t frame[64];
    KymoTraceEnvelopeInfo info;
    info.session_id = 0x0102030405060708ULL;
    info.packet_seq = 7u;
    info.payload_kind = KYMO_TRACE_PAYLOAD_BATCH;
    size_t payloadLength = KymoTrace_EncodeBatch(1u, &event, 1u, payload, sizeof(payload));
    size_t frameLength =
        KymoTrace_EncodeEnvelope(&info, payload, payloadLength, frame, sizeof(frame));
    for (size_t i = 0u; i < frameLength; ++i)
    {
        (void)printf("%02x", frame[i]);
    }
    (void)printf("\n");
    return 0;
}
