/* SPDX-License-Identifier: MIT */
/** @file endpoint_c_test.c
 * @brief C99 consumer of independent base and Plot headers.
 */
#include <endpoint/kymo_endpoint.h>
#include <endpoint/features/kymo_plot.h>
#include <assert.h>
static size_t writes;
static uint8_t storage[KYMO_FRAME_MAX_SIZE];
static const KymoChannel channels[] = {{0u, 7u, 0u}};
/*******************************************************************************
 * clockMs
 ******************************************************************************/
static uint32_t clockMs(void *user)
{
    (void)user;
    return 1u;
}
/*******************************************************************************
 * sample
 ******************************************************************************/
static uint8_t sample(void *user, uint8_t id, KymoSample *output)
{
    (void)user;
    assert(id == channels[0].id);
    output->value = 1.0f;
    return 1u;
}
/*******************************************************************************
 * writeBytes
 ******************************************************************************/
static size_t writeBytes(void *user, const uint8_t *bytes, size_t length)
{
    (void)user;
    assert(bytes != NULL && length > 0u);
    ++writes;
    return length;
}
/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    KymoEndpoint endpoint = {0};
    KymoEndpointPlot plot = {0};
    KymoEndpointConfig base = {0};
    KymoEndpointPlotConfig config = {0};
    base.tx.buffer = storage;
    base.tx.capacity = sizeof(storage);
    base.tx.write = writeBytes;
    base.clock_ms = clockMs;
    config.channels = channels;
    config.channel_count = KYMO_COUNT_OF(channels);
    config.sample = sample;
    assert(KymoEndpoint_Init(&endpoint, &base) == KYMO_OK);
    assert(KymoEndpointPlot_Init(&endpoint, &plot, &config) == KYMO_OK);
    assert(KymoEndpoint_Main(&endpoint) == KYMO_BUSY && writes == 0u);
    assert(KymoEndpoint_Main(&endpoint) == KYMO_BUSY && writes == 1u);
    assert(KymoEndpoint_RequestStop(&endpoint) == KYMO_OK);
    assert(KymoEndpoint_Main(&endpoint) == KYMO_IDLE && writes == 2u);
    return 0;
}
