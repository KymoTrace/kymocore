/* SPDX-License-Identifier: MIT */
/** @file kymo_init.cpp
 * @brief Common resources and port adapters; no feature configuration here.
 */
#include "kymo_init.h"
#include "detail/kymo_internal.h"
#include "kymo_port.h"
static const size_t RX_BUDGET_BYTES = 16u;
static KymoEndpoint sEndpoint;
static uint8_t sTxBuffer[EXAMPLE_TX_CAPACITY];
static uint8_t sRxBuffer[RX_BUDGET_BYTES];
/*******************************************************************************
 * exampleEndpoint
 ******************************************************************************/
KymoEndpoint *exampleEndpoint(void)
{
    return &sEndpoint;
}
/*******************************************************************************
 * writePort
 ******************************************************************************/
static size_t writePort(void *user, const uint8_t *bytes, size_t length)
{
    (void)user;
    return examplePortWrite(bytes, length);
}
/*******************************************************************************
 * readPort
 ******************************************************************************/
static size_t readPort(void *user, uint8_t *bytes, size_t capacity)
{
    (void)user;
    return examplePortRead(bytes, capacity);
}
/*******************************************************************************
 * busyPort
 ******************************************************************************/
static uint8_t busyPort(void *user)
{
    (void)user;
    return examplePortBusy();
}
/*******************************************************************************
 * servicePort
 ******************************************************************************/
static void servicePort(void *user)
{
    (void)user;
    examplePortService();
}
/*******************************************************************************
 * exampleCoreConfig
 ******************************************************************************/
KymoEndpointConfig exampleCoreConfig(void)
{
    KymoEndpointConfig config = {};
    config.tx.buffer = sTxBuffer;
    config.tx.capacity = sizeof(sTxBuffer);
    config.tx.write = writePort;
    config.tx.busy = busyPort;
    config.read = readPort;
    config.service = servicePort;
    config.rx_buffer = sRxBuffer;
    config.rx_capacity = sizeof(sRxBuffer);
    config.clock_ms = examplePortClockMs;
    return config;
}
/*******************************************************************************
 * KymoApp_Init
 ******************************************************************************/
KymoStatus KymoApp_Init(const KymoEndpointConfig *config)
{
    return KymoEndpoint_Init(&sEndpoint, config);
}
/*******************************************************************************
 * KymoApp_Main
 ******************************************************************************/
KymoStatus KymoApp_Main(void)
{
    return KymoEndpoint_Main(&sEndpoint);
}
/*******************************************************************************
 * KymoApp_RequestStop
 ******************************************************************************/
KymoStatus KymoApp_RequestStop(void)
{
    return KymoEndpoint_RequestStop(&sEndpoint);
}
