/* SPDX-License-Identifier: MIT */
/* Independent transport ownership regression cases. */
#include <assert.h>
#include <string.h>
#include "transport/kymo_mux.h"

static uint64_t nowTicks;
static uint64_t clockRead(void *)
{
    return ++nowTicks;
}
static uint8_t coreRead(void *)
{
    return 0u;
}
static uint32_t contextRead(void *)
{
    return 1u;
}
struct Driver
{
    uint8_t busy;
    uint8_t borrow;
    uint8_t overAccept;
    size_t limit;
    size_t used;
    unsigned calls;
    uint8_t received[1024];
    const uint8_t *lent;
    size_t lentLength;
};
static uint8_t isBusy(void *user)
{
    return static_cast<Driver *>(user)->busy;
}
static size_t writeBytes(void *user, const uint8_t *bytes, size_t length)
{
    Driver *driver = static_cast<Driver *>(user);
    ++driver->calls;
    size_t accepted = length < driver->limit ? length : driver->limit;
    if (driver->overAccept != 0u)
    {
        accepted = length + 1u;
    }
    else
    {
        assert(driver->used + accepted <= sizeof(driver->received));
        memcpy(driver->received + driver->used, bytes, accepted);
        driver->used += accepted;
        if (driver->borrow != 0u)
        {
            driver->busy = 1u;
            driver->lent = bytes;
            driver->lentLength = accepted;
        }
    }
    return accepted;
}
int main(void)
{
    KymoTraceRecorder recorder;
    KymoTraceConfig config;
    memset(&config, 0, sizeof(config));
    config.clock_ticks = clockRead;
    config.core_id = coreRead;
    config.current_context = contextRead;
    assert(KymoTrace_Init(&recorder, &config) == KYMO_OK);
    Driver driver;
    memset(&driver, 0, sizeof(driver));
    KymoMuxConfig transport;
    memset(&transport, 0, sizeof(transport));
    transport.recorder = &recorder;
    transport.write = writeBytes;
    transport.busy = isBusy;
    transport.transport_user = &driver;
    transport.max_frame_size = 64u;
    KymoMux mux;
    assert(KymoMux_Init(&mux, &transport) == KYMO_OK);
    KymoTrace_Mark(&recorder, 1u, 1u, 7u);
    /* Zero acceptance retries the exact prefix, then two-byte short writes. */
    assert(KymoMux_Service(&mux) == KYMO_BUSY);
    assert(driver.used == 0u && mux.pending != 0u);
    uint8_t expected[240];
    const size_t expectedLength = mux.frame_length;
    memcpy(expected, mux.frame, expectedLength);
    driver.limit = 2u;
    for (unsigned i = 0u; i < 120u && mux.pending != 0u; ++i)
    {
        (void)KymoMux_Service(&mux);
    }
    assert(driver.used == expectedLength);
    assert(memcmp(expected, driver.received, expectedLength) == 0);
    /* Driver borrows the whole frame and stays busy while new records arrive. */
    driver.limit = 240u;
    driver.borrow = 1u;
    KymoTrace_Mark(&recorder, 1u, 1u, 8u);
    assert(KymoMux_Service(&mux) == KYMO_BUSY);
    assert(driver.busy != 0u && mux.pending != 0u);
    memcpy(expected, driver.lent, driver.lentLength);
    KymoTrace_Mark(&recorder, 1u, 1u, 9u);
    unsigned previousCalls = driver.calls;
    for (unsigned i = 0u; i < 8u; ++i)
    {
        assert(KymoMux_Service(&mux) == KYMO_BUSY);
        assert(memcmp(expected, driver.lent, driver.lentLength) == 0);
    }
    assert(driver.calls == previousCalls);
    driver.busy = driver.borrow = 0u;
    assert(KymoMux_Service(&mux) == KYMO_OK);
    assert(driver.calls == previousCalls);
    /* An impossible driver byte count permanently faults the transport. */
    driver.overAccept = 1u;
    assert(KymoMux_Service(&mux) == KYMO_IO_ERROR);
    previousCalls = driver.calls;
    assert(KymoMux_Service(&mux) == KYMO_IO_ERROR);
    assert(driver.calls == previousCalls);
    return 0;
}
