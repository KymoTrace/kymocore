/* SPDX-License-Identifier: MIT */
/** @file app_test.cpp
 * @brief Run the application with a different port, without files/native startup.
 */
#include "kymo_app.h"
#include "kymo_port.h"
#include <assert.h>

static uint64_t sTicks = 0u;
static size_t sWrites = 0u;

/*******************************************************************************
 * examplePortRead
 ******************************************************************************/
size_t examplePortRead(uint8_t *output, size_t capacity)
{
    (void)output;
    (void)capacity;
    return 0u;
}

/*******************************************************************************
 * examplePortWrite
 ******************************************************************************/
size_t examplePortWrite(const uint8_t *bytes, size_t length)
{
    assert(bytes != nullptr && length != 0u);
    ++sWrites;
    return length;
}

/*******************************************************************************
 * examplePortBusy
 ******************************************************************************/
uint8_t examplePortBusy(void)
{
    return 0u;
}

/*******************************************************************************
 * examplePortClockMs
 ******************************************************************************/
uint32_t examplePortClockMs(void *user)
{
    (void)user;
    return (uint32_t)sTicks;
}

/*******************************************************************************
 * examplePortClockTicks
 ******************************************************************************/
uint64_t examplePortClockTicks(void *user)
{
    (void)user;
    return sTicks;
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    KymoStatus status = KYMO_BUSY;
    size_t writes = 0u;
    assert(exampleAppCycle() == KYMO_BAD_CONFIG);
    assert(exampleAppDrain() == KYMO_BAD_CONFIG);
    assert(exampleAppInit() == KYMO_OK);
    for (uint32_t cycle = 0u; cycle < 100u; ++cycle) {
        ++sTicks;
        assert(exampleAppCycle() == KYMO_OK);
    }
    assert(sWrites > 0u);
    assert(exampleAppInit() == KYMO_REJECTED);
    for (uint32_t cycle = 0u; (cycle < 1000u) && (status == KYMO_BUSY); ++cycle) {
        ++sTicks;
        status = exampleAppDrain();
        assert(exampleAppCycle() == KYMO_REJECTED);
    }
    assert(status == KYMO_IDLE);
    writes = sWrites;
    assert(exampleAppDrain() == KYMO_IDLE && sWrites == writes);
    assert(exampleAppInit() == KYMO_REJECTED);
    return 0;
}
