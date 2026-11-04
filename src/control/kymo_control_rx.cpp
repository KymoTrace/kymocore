/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../kymo_config.h"
#if KYMO_EFFECTIVE_PARAMETERS || KYMO_EFFECTIVE_WATCH
#include "kymo_control_rx.h"

/*******************************************************************************
 * restart_at
 ******************************************************************************/
/* After a lost sync, a sync byte may already start the next frame. */
static void restart_at(KymoControlRx *rx, uint8_t byte)
{
    rx->expected = 0u;
    rx->length = (byte == KYMO_SYNC_0) ? 1u : 0u;
    rx->frame[0] = byte;
}

/*******************************************************************************
 * accept_descriptor
 ******************************************************************************/
/* A control frame states its length in the next byte; a DATA frame in its descriptor. */
static void accept_descriptor(KymoControlRx *rx, uint8_t byte, uint32_t *invalid)
{
    rx->expected = (byte == KYMO_DESCRIPTOR_PARAM) ? 0u : KymoProtocol_FrameLength(byte);
    if ((rx->expected == 0u) && (byte != KYMO_DESCRIPTOR_PARAM)) {
        ++*invalid;
        restart_at(rx, byte);
    } else {
        rx->frame[2] = byte;
        rx->length = 3u;
    }
}

/*******************************************************************************
 * accept_control_length
 ******************************************************************************/
static void accept_control_length(KymoControlRx *rx, uint8_t byte, uint32_t *invalid)
{
    if ((byte == 0u) || (byte > KYMO_CONTROL_REQUEST_MAX)) {
        ++*invalid;
        restart_at(rx, byte);
    } else {
        rx->frame[3] = byte;
        rx->length = 4u;
        rx->expected = 5u + byte;
    }
}

/*******************************************************************************
 * KymoControlRx_Byte
 ******************************************************************************/
size_t KymoControlRx_Byte(KymoControlRx *rx, uint8_t byte, uint32_t *invalid)
{
    size_t complete = 0u;
    if (rx->length == 0u) {
        restart_at(rx, byte);
    } else if (rx->length == 1u) {
        if (byte == KYMO_SYNC_1) {
            rx->frame[1] = byte;
            rx->length = 2u;
        } else {
            restart_at(rx, byte);
        }
    } else if (rx->length == 2u) {
        accept_descriptor(rx, byte, invalid);
    } else if (rx->expected == 0u) {
        accept_control_length(rx, byte, invalid);
    } else {
        rx->frame[rx->length] = byte;
        ++rx->length;
        if (rx->length == rx->expected) {
            /* frame stays readable until the next byte arrives. */
            complete = rx->length;
            rx->length = 0u;
            rx->expected = 0u;
        }
    }
    return complete;
}

/*******************************************************************************
 * KymoControlRx_CrcValid
 ******************************************************************************/
uint8_t KymoControlRx_CrcValid(const uint8_t *frame, size_t length)
{
    return (uint8_t)(KymoProtocol_Crc8(frame + 2, length - 3u) == frame[length - 1u]);
}

/*******************************************************************************
 * KymoControl_IsKnownOperation
 ******************************************************************************/
uint8_t KymoControl_IsKnownOperation(uint8_t operation)
{
    return (uint8_t)((operation >= KYMO_CONTROL_OP_FIRST) && (operation <= KYMO_CONTROL_OP_LAST));
}

#endif /* KYMO_EFFECTIVE_PARAMETERS || KYMO_EFFECTIVE_WATCH */
