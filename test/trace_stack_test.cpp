/* SPDX-License-Identifier: MIT */
/* Stack pointer sampling: ENTER/EXIT carry the stack address when compiled in. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "trace/kymo_trace.h"

static uint64_t ticks;
static KymoTraceRecorder recorder;

/*******************************************************************************
 * clockRead
 ******************************************************************************/
static uint64_t clockRead(void *user)
{
    (void)user;
    return ticks++;
}

/** @brief Enter a scope one frame deeper than the caller; volatile keeps the frame. */
/*******************************************************************************
 * nested
 ******************************************************************************/
static void __attribute__((noinline)) nested(void)
{
    volatile uint8_t frame[256];
    frame[0] = 1u;
    KymoTrace_Enter(&recorder, 1u, 2u);
    KymoTrace_Exit(&recorder, 1u, 2u);
    frame[255] = frame[0];
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    KymoTraceConfig config;
    KymoTraceEvent outer;
    KymoTraceEvent inner;
    KymoTraceEvent event;
    memset(&config, 0, sizeof(config));
    config.clock_ticks = clockRead;
    config.session_id = 1u;
    config.clock_domain_id = 1u;
    assert(KymoTrace_Init(&recorder, &config) == KYMO_OK);
    KymoTrace_Enter(&recorder, 1u, 1u);
    nested();
    KymoTrace_Exit(&recorder, 1u, 1u);
    KymoTrace_Mark(&recorder, 1u, 3u, 7u);
    assert(KymoTrace_Pop(&recorder, &outer) == KYMO_OK);
    assert(KymoTrace_Pop(&recorder, &inner) == KYMO_OK);
    assert((outer.flags & KYMO_TRACE_FLAG_STACK_SAMPLE) != 0u);
    assert((inner.flags & KYMO_TRACE_FLAG_STACK_SAMPLE) != 0u);
    assert((outer.flags & KYMO_TRACE_FLAG_ARG_VALID) == 0u);
    assert((outer.arg != 0u) && (inner.arg != 0u));
    /* The stack grows down on the test host: the nested scope sits lower. */
    assert(inner.arg < outer.arg);
    assert((outer.arg - inner.arg) >= 256u);
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK);
    assert((event.event_type == KYMO_TRACE_EVENT_EXIT) && (event.arg == inner.arg));
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK);
    assert((event.event_type == KYMO_TRACE_EVENT_EXIT) && (event.arg == outer.arg));
    assert(KymoTrace_Pop(&recorder, &event) == KYMO_OK);
    /* Markers keep their own value and do not claim a stack sample. */
    assert((event.arg == 7u) && ((event.flags & KYMO_TRACE_FLAG_STACK_SAMPLE) == 0u));
    return 0;
}
