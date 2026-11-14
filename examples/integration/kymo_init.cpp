/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_init.cpp
 * @brief Common resources and port adapters; no feature configuration here.
 */
#include "kymo_init.h"
#include "detail/kymo_internal.h"
#include "kymo_port.h"
static const size_t RX_BUDGET_BYTES = 16u;
static KymoCoreEndpoint sEndpoint;
static uint8_t sTxBuffer[EXAMPLE_TX_CAPACITY];
static uint8_t sRxBuffer[RX_BUDGET_BYTES];

/****************************************************************************************************
 * exampleEndpoint
 ***************************************************************************************************/
KymoCoreEndpoint *exampleEndpoint(void)
{
    return &sEndpoint;
}

/****************************************************************************************************
 * writePort
 ***************************************************************************************************/
static size_t writePort(void *user, const uint8_t *bytes, size_t length)
{
    (void)user;
    return examplePortWrite(bytes, length);
}

/****************************************************************************************************
 * readPort
 ***************************************************************************************************/
static size_t readPort(void *user, uint8_t *bytes, size_t capacity)
{
    (void)user;
    return examplePortRead(bytes, capacity);
}

/****************************************************************************************************
 * busyPort
 ***************************************************************************************************/
static uint8_t busyPort(void *user)
{
    (void)user;
    return examplePortBusy();
}

/****************************************************************************************************
 * servicePort
 ***************************************************************************************************/
static void servicePort(void *user)
{
    (void)user;
    examplePortService();
}

/** @brief Fixed core resources, callbacks and feature settings. */
static const KymoCoreEndpointConfig sCoreConfig = {
    {sTxBuffer, sizeof(sTxBuffer), writePort, busyPort, nullptr}, /* TX */
    readPort,
    servicePort,
    sRxBuffer,
    sizeof(sRxBuffer),
    examplePortClockMs,
    nullptr};

/****************************************************************************************************
 * exampleCoreConfig
 ***************************************************************************************************/
const KymoCoreEndpointConfig &exampleCoreConfig(void)
{
    return sCoreConfig;
}

/****************************************************************************************************
 * KymoApp_Init
 ***************************************************************************************************/
KymoStatus KymoApp_Init(const KymoCoreEndpointConfig *config)
{
    return KymoCoreEndpoint_init(&sEndpoint, config);
}

/****************************************************************************************************
 * KymoApp_Main
 ***************************************************************************************************/
KymoStatus KymoApp_Main(void)
{
    return KymoCoreEndpoint_main(&sEndpoint);
}

/****************************************************************************************************
 * KymoApp_RequestStop
 ***************************************************************************************************/
KymoStatus KymoApp_RequestStop(void)
{
    return KymoCoreEndpoint_requestStop(&sEndpoint);
}
