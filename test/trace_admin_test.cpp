/* SPDX-License-Identifier: MIT */
/** @file trace_admin_test.cpp
 * @brief Administrative startup preserves queued events and borrowed fragments.
 */
#include <transport/kymo_mux.h>
#include <assert.h>
#include <string.h>

static uint8_t busy;
static uint8_t hold;
static uint8_t reject;
static size_t writes;
static const uint8_t *borrowed;
static uint8_t snapshot[KYMO_TRACE_FRAME_MAX];
static size_t snapshotLength;
/*******************************************************************************
 * clockTicks
 ******************************************************************************/
static uint64_t clockTicks(void *)
{
    return 1u;
}
/*******************************************************************************
 * busyPort
 ******************************************************************************/
static uint8_t busyPort(void *)
{
    return busy;
}
/*******************************************************************************
 * writePort
 ******************************************************************************/
static size_t writePort(void *, const uint8_t *bytes, size_t length)
{
    size_t accepted = reject != 0u ? 0u : 1u;
    assert(length != 0u && busy == 0u);
    ++writes;
    if ((hold != 0u) && (accepted != 0u)) {
        borrowed = bytes;
        snapshotLength = length;
        memcpy(snapshot, bytes, length);
        busy = 1u;
    }
    return accepted;
}
/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    KymoTraceRecorder recorder = {};
    KymoTraceConfig trace = {};
    trace.clock_ticks = clockTicks;
    trace.session_id = 1u;
    assert(KymoTrace_Init(&recorder, &trace) == KYMO_OK);
    KymoMux mux = {};
    KymoMuxConfig config = {};
    config.recorder = &recorder;
    config.write = writePort;
    config.busy = busyPort;
    assert(KymoMux_Init(&mux, &config) == KYMO_OK);
    assert(KymoMux_ServiceAdmin(nullptr) == KYMO_BAD_CONFIG);
    KymoTrace_Mark(&recorder, 1u, 1u, 1u);
    assert(KymoMux_ServiceAdmin(&mux) == KYMO_IDLE && writes == 0u);
    const uint8_t metadata[] = "{}";
    assert(KymoMux_QueueAdmin(&mux, KYMO_TRACE_PAYLOAD_SESSION_META,
        1u, metadata, sizeof(metadata) - 1u) == KYMO_OK);
    reject = 1u;
    assert(KymoMux_ServiceAdmin(&mux) == KYMO_BUSY && mux.frame_offset == 0u);
    reject = 0u;
    hold = 1u;
    assert(KymoMux_ServiceAdmin(&mux) == KYMO_BUSY);
    size_t calls = writes;
    assert(KymoMux_ServiceAdmin(&mux) == KYMO_BUSY && writes == calls);
    assert(memcmp(borrowed, snapshot, snapshotLength) == 0);
    busy = 0u;
    hold = 0u;
    for (size_t cycle = 0u; cycle < KYMO_TRACE_FRAME_MAX; ++cycle) {
        (void)KymoMux_ServiceAdmin(&mux);
    }
    assert(mux.admin_pending == 0u && mux.pending == 0u);
    calls = writes;
    assert(KymoMux_ServiceAdmin(&mux) == KYMO_IDLE && writes == calls);
    /* The early event survived admin-only processing; a partial event frame
       must be completed by the regular service, never by admin-only service. */
    assert(KymoMux_Service(&mux) == KYMO_BUSY && writes == calls + 1u);
    assert(mux.frame[3] == KYMO_TRACE_PAYLOAD_BATCH);
    calls = writes;
    assert(KymoMux_ServiceAdmin(&mux) == KYMO_REJECTED && writes == calls);
    for (size_t cycle = 0u; cycle < KYMO_TRACE_FRAME_MAX; ++cycle) {
        (void)KymoMux_Service(&mux);
    }
    assert(mux.pending == 0u && KymoMux_Service(&mux) == KYMO_IDLE);
    return 0;
}
