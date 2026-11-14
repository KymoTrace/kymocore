/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../kymo_config.h"
#if KYMO_EFFECTIVE_PARAMETERS || KYMO_EFFECTIVE_WATCH
#include "kymo_control_rx.h"

/****************************************************************************************************
 * KymoControlRx_restartAt
 ***************************************************************************************************/
/* After a lost sync, a sync byte may already start the next frame. */
static void KymoControlRx_restartAt(KymoControlRx *rx, uint8_t byte)
{
    rx->expected = 0u;
    rx->length = (byte == KYMO_SYNC_0) ? 1u : 0u;
    rx->frame[0] = byte;
}

/****************************************************************************************************
 * KymoControlRx_acceptDescriptor
 ***************************************************************************************************/
/* A control frame states its length in the next byte; a DATA frame in its descriptor. */
static void KymoControlRx_acceptDescriptor(KymoControlRx *rx, uint8_t byte, uint32_t *invalid)
{
    rx->expected = (byte == KYMO_DESCRIPTOR_PARAM) ? 0u : KymoProtocol_frameLength(byte);
    if ((rx->expected == 0u) && (byte != KYMO_DESCRIPTOR_PARAM))
    {
        ++*invalid;
        KymoControlRx_restartAt(rx, byte);
    }
    else
    {
        rx->frame[2] = byte;
        rx->length = 3u;
    }
}

/****************************************************************************************************
 * KymoControlRx_acceptControlLength
 ***************************************************************************************************/
static void KymoControlRx_acceptControlLength(KymoControlRx *rx, uint8_t byte, uint32_t *invalid)
{
    if ((byte == 0u) || (byte > KYMO_CONTROL_REQUEST_MAX))
    {
        ++*invalid;
        KymoControlRx_restartAt(rx, byte);
    }
    else
    {
        rx->frame[3] = byte;
        rx->length = 4u;
        rx->expected = 5u + byte;
    }
}

/****************************************************************************************************
 * KymoControlRx_byte
 ***************************************************************************************************/
size_t KymoControlRx_byte(KymoControlRx *rx, uint8_t byte, uint32_t *invalid)
{
    size_t complete = 0u;
    if (rx->length == 0u)
    {
        KymoControlRx_restartAt(rx, byte);
    }
    else if (rx->length == 1u)
    {
        if (byte == KYMO_SYNC_1)
        {
            rx->frame[1] = byte;
            rx->length = 2u;
        }
        else
        {
            KymoControlRx_restartAt(rx, byte);
        }
    }
    else if (rx->length == 2u)
    {
        KymoControlRx_acceptDescriptor(rx, byte, invalid);
    }
    else if (rx->expected == 0u)
    {
        KymoControlRx_acceptControlLength(rx, byte, invalid);
    }
    else
    {
        rx->frame[rx->length] = byte;
        ++rx->length;
        if (rx->length == rx->expected)
        {
            /* frame stays readable until the next byte arrives. */
            complete = rx->length;
            rx->length = 0u;
            rx->expected = 0u;
        }
    }
    return complete;
}

/****************************************************************************************************
 * KymoControlRx_crcValid
 ***************************************************************************************************/
uint8_t KymoControlRx_crcValid(const uint8_t *frame, size_t length)
{
    return (uint8_t)(KymoProtocol_crc8(frame + 2, length - 3u) == frame[length - 1u]);
}

/****************************************************************************************************
 * KymoControlRx_isKnownOperation
 ***************************************************************************************************/
uint8_t KymoControlRx_isKnownOperation(uint8_t operation)
{
    return (uint8_t)((operation >= KYMO_CONTROL_OP_FIRST) && (operation <= KYMO_CONTROL_OP_LAST));
}

#endif /* KYMO_EFFECTIVE_PARAMETERS || KYMO_EFFECTIVE_WATCH */
