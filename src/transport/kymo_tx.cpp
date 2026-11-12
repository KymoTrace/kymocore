/* SPDX-License-Identifier: MIT */
/** @file kymo_tx.cpp
 * @brief Bounded frame copying and physical transport ownership.
 */
#include "kymo_tx.h"
#if KYMO_EFFECTIVE_TX
#include <string.h>

/*******************************************************************************
 * KymoTx_Init
 ******************************************************************************/
KymoStatus KymoTx_Init(KymoTx *tx, const KymoTxConfig *config)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (tx != nullptr)
    {
        if (tx->initialized != 0u)
        {
            status = KYMO_REJECTED;
        }
        else if ((config != nullptr) && (config->buffer != nullptr) && (config->capacity != 0u) &&
                 (config->write != nullptr))
        {
            tx->config = *config;
            tx->length = 0u;
            tx->offset = 0u;
            tx->fault = 0u;
            tx->initialized = 1u;
            status = KYMO_OK;
        }
    }
    return status;
}

/*******************************************************************************
 * KymoTx_Submit
 ******************************************************************************/
KymoStatus KymoTx_Submit(KymoTx *tx, const uint8_t *bytes, size_t length)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if ((tx != nullptr) && (tx->initialized != 0u))
    {
        if (tx->fault != 0u)
        {
            status = KYMO_IO_ERROR;
        }
        else if ((bytes != nullptr) && (length != 0u) && (length <= tx->config.capacity))
        {
            status = KYMO_BUSY;
            if (tx->length == 0u)
            {
                memmove(tx->config.buffer, bytes, length);
                tx->length = length;
                tx->offset = 0u;
                status = KYMO_OK;
            }
        }
    }
    return status;
}

/*******************************************************************************
 * transportBusy
 ******************************************************************************/
static uint8_t transportBusy(const KymoTx *tx)
{
    uint8_t busy = 0u;
    if (tx->config.busy != nullptr)
    {
        busy = tx->config.busy(tx->config.transport_user);
    }
    return busy;
}

/*******************************************************************************
 * KymoTx_Service
 ******************************************************************************/
KymoStatus KymoTx_Service(KymoTx *tx)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if ((tx != nullptr) && (tx->initialized != 0u))
    {
        if ((tx->fault == 0u) && (tx->length != 0u) && (transportBusy(tx) == 0u))
        {
            if (tx->offset < tx->length)
            {
                size_t remaining = tx->length - tx->offset;
                size_t accepted = tx->config.write(tx->config.transport_user,
                                                   tx->config.buffer + tx->offset, remaining);
                if (accepted > remaining)
                {
                    tx->fault = 1u;
                }
                else
                {
                    tx->offset += accepted;
                }
            }
            if ((tx->fault == 0u) && (tx->offset == tx->length) && (transportBusy(tx) == 0u))
            {
                tx->length = 0u;
                tx->offset = 0u;
            }
        }
        status = (tx->fault != 0u) ? KYMO_IO_ERROR : ((tx->length == 0u) ? KYMO_IDLE : KYMO_BUSY);
    }
    return status;
}
#endif /* KYMO_EFFECTIVE_TX */
