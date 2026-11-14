/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core.cpp
 * @brief Default connection ownership and public lifecycle.
 */
#include "../kymo_core.h"
#include "detail/kymo_core_internal.h"
#if KYMO_EFFECTIVE_CORE
static KymoCoreEndpoint defaultConnection = {};

/****************************************************************************************************
 * KymoCore_getEndpoint
 ***************************************************************************************************/
/* Return stable storage without allocating or registering anything. */
KymoCoreEndpoint *KymoCore_getEndpoint(void)
{
    return &defaultConnection;
}

/****************************************************************************************************
 * KymoCore_init
 ***************************************************************************************************/
/* Translate public transport settings; dispatcher types stay internal. */
KymoStatus KymoCore_init(const KymoCoreConfig *pConfig)
{
    KymoStatus status = KYMO_REJECTED;
    KymoCoreEndpointConfig internalConfig = {};
    if (defaultConnection.phase == KYMO_ENDPOINT_UNINITIALIZED)
    {
        if (pConfig == nullptr)
        {
            defaultConnection.phase = KYMO_ENDPOINT_CONFIGURING;
            status = KYMO_OK;
        }
        else
        {
            internalConfig.tx.buffer = pConfig->pTxBufferAv;
            internalConfig.tx.capacity = pConfig->txBufferSizeBytes;
            internalConfig.tx.write = pConfig->writeBytesCbk;
            internalConfig.tx.busy = pConfig->isBusyCbk;
            internalConfig.tx.transport_user = pConfig->pTransportContext;
            internalConfig.read = pConfig->readBytesCbk;
            internalConfig.service = pConfig->serviceCbk;
            internalConfig.rx_buffer = pConfig->pRxBufferAv;
            internalConfig.rx_capacity = pConfig->rxBufferSizeBytes;
            internalConfig.clock_ms = pConfig->clockMsCbk;
            internalConfig.clock_user = pConfig->pClockContext;
            status = KymoCoreEndpoint_init(&defaultConnection, &internalConfig);
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoCore_main
 ***************************************************************************************************/
/* Empty core is valid; nonempty processing reuses the existing dispatcher. */
KymoStatus KymoCore_main(void)
{
    KymoStatus status = KYMO_IDLE;
    if ((defaultConnection.count == 0u) &&
        ((defaultConnection.phase == KYMO_ENDPOINT_CONFIGURING) ||
         (defaultConnection.phase == KYMO_ENDPOINT_RUNNING)))
    {
        defaultConnection.phase = KYMO_ENDPOINT_RUNNING;
    }
    else
    {
        status = KymoCoreEndpoint_main(&defaultConnection);
    }
    return status;
}
#endif
