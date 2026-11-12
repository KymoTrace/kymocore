/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <string.h>
#include <type_traits>
#include "trace/kymo_instrumentation.h"
static uint64_t ticks;
static unsigned recorderReads;
static unsigned contextReads;
static unsigned symbolReads;
static KymoTraceRecorder recorder;
static uint64_t clockRead(void *)
{
    return ticks++;
}
static uint8_t coreRead(void *)
{
    return 0u;
}
static uint32_t contextRead(void *)
{
    return 1u;
}
static KymoTraceRecorder *getRecorder(void)
{
    ++recorderReads;
    return &recorder;
}
static uint32_t getContext(void)
{
    ++contextReads;
    return 1u;
}
static uint16_t getSymbol(void)
{
    ++symbolReads;
    return 2u;
}
static int measuredReturn(void)
{
    KYMO_TRACE_SCOPE(getRecorder(), getContext(), getSymbol());
    KYMO_TRACE_SCOPE(getRecorder(), getContext(), getSymbol());
    return 17;
}
int main(void)
{
    static_assert(!std::is_copy_constructible<KymoTraceScope>::value, "Scope must not copy");
    KymoTraceConfig config;
    memset(&config, 0, sizeof(config));
    config.clock_ticks = clockRead;
    config.core_id = coreRead;
    config.current_context = contextRead;
    assert(KymoTrace_Init(&recorder, &config) == KYMO_OK);
    assert(measuredReturn() == 17);
    assert(recorderReads == 2u && contextReads == 2u && symbolReads == 2u);
    KymoTraceEvent event;
    const uint8_t types[] = {1u, 1u, 2u, 2u};
    const uint16_t depths[] = {1u, 2u, 2u, 1u};
    for (unsigned i = 0u; i < 4u; ++i)
    {
        assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK);
        assert(event.event_type == types[i] && event.depth == depths[i]);
    }
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_IDLE);
    return 0;
}
