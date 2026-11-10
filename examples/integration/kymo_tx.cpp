/* SPDX-License-Identifier: MIT */
/** @file kymo_tx.cpp
 * @brief One frame copy, partial writes and explicit port buffer ownership.
 */
#include "kymo_tx.h"
#include "kymo_port.h"
#include <string.h>

static uint8_t sFrame[EXAMPLE_TX_CAPACITY];
static size_t sLength = 0u;
static size_t sOffset = 0u;
static uint8_t sFault = 0u;

/*******************************************************************************
 * exampleTxSubmit
 ******************************************************************************/
KymoStatus exampleTxSubmit(const uint8_t *bytes, size_t length)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (sFault != 0u) {
        status = KYMO_IO_ERROR;
    } else if ((bytes != nullptr) && (length != 0u) && (length <= sizeof(sFrame))) {
        status = KYMO_BUSY;
        if (sLength == 0u) {
            memcpy(sFrame, bytes, length);
            sLength = length;
            sOffset = 0u;
            status = KYMO_OK;
        }
    }
    return status;
}

/*******************************************************************************
 * exampleTxService
 ******************************************************************************/
KymoStatus exampleTxService(void)
{
    KymoStatus status = KYMO_BUSY;
    if ((sFault == 0u) && (sLength != 0u) && (examplePortBusy() == 0u)) {
        if (sOffset < sLength) {
            size_t remaining = sLength - sOffset;
            size_t accepted = examplePortWrite(sFrame + sOffset, remaining);
            if (accepted > remaining) {
                sFault = 1u;
            } else {
                sOffset += accepted;
            }
        }
        if ((sFault == 0u) && (sOffset == sLength) && (examplePortBusy() == 0u)) {
            sLength = 0u;
            sOffset = 0u;
        }
    }
    if (sFault != 0u) {
        status = KYMO_IO_ERROR;
    } else if (sLength == 0u) {
        status = KYMO_IDLE;
    }
    return status;
}
