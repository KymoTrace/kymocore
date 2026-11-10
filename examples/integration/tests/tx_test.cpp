/* SPDX-License-Identifier: MIT */
/** @file tx_test.cpp
 * @brief Sender contract tests with an independently controlled borrowing port.
 */
#include "kymo_tx.h"
#include "kymo_port.h"
#include <assert.h>
#include <string.h>

/* Explicit fixture bytes, split into two physical transfers. */
static const uint8_t EXPECTED_FRAME[] = { 1u, 2u, 3u, 4u };
static const size_t TRANSFER_CHUNK_BYTES = 2u;
static const uint8_t REUSED_BUFFER_BYTE = 99u;

static uint8_t sBusy = 0u;
static uint8_t sRejectOnce = 1u;
static uint8_t sViolate = 0u;
static size_t sWriteCalls = 0u;
static const uint8_t *sBorrowed = nullptr;
static size_t sBorrowedLength = 0u;
static uint8_t sWire[EXAMPLE_TX_CAPACITY + sizeof(EXPECTED_FRAME)];
static size_t sWireLength = 0u;

/*******************************************************************************
 * examplePortBusy
 ******************************************************************************/
uint8_t examplePortBusy(void)
{
    return sBusy;
}

/*******************************************************************************
 * examplePortWrite
 ******************************************************************************/
size_t examplePortWrite(const uint8_t *bytes, size_t length)
{
    size_t accepted = 0u;
    assert(bytes != nullptr && length != 0u && sBusy == 0u);
    ++sWriteCalls;
    if (sViolate != 0u) {
        accepted = length + 1u;
    } else if (sRejectOnce != 0u) {
        sRejectOnce = 0u;
    } else {
        accepted = (length > TRANSFER_CHUNK_BYTES) ? TRANSFER_CHUNK_BYTES : length;
        sBorrowed = bytes;
        sBorrowedLength = accepted;
        sBusy = 1u;
    }
    return accepted;
}

/*******************************************************************************
 * releaseTransfer
 ******************************************************************************/
static void releaseTransfer(void)
{
    assert(sBorrowed != nullptr);
    assert(sWireLength + sBorrowedLength <= sizeof(sWire));
    memcpy(sWire + sWireLength, sBorrowed, sBorrowedLength);
    sWireLength += sBorrowedLength;
    sBorrowed = nullptr;
    sBorrowedLength = 0u;
    sBusy = 0u;
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    uint8_t original[sizeof(EXPECTED_FRAME)] = {};
    uint8_t maximum[EXAMPLE_TX_CAPACITY] = {};
    size_t calls = 0u;
    memcpy(original, EXPECTED_FRAME, sizeof(original));
    assert(exampleTxService() == KYMO_IDLE);
    assert(exampleTxSubmit(nullptr, sizeof(original)) == KYMO_BAD_CONFIG);
    assert(exampleTxSubmit(original, 0u) == KYMO_BAD_CONFIG);
    assert(exampleTxSubmit(original, EXAMPLE_TX_CAPACITY + 1u) == KYMO_BAD_CONFIG);
    assert(exampleTxSubmit(original, sizeof(original)) == KYMO_OK);
    memset(original, REUSED_BUFFER_BYTE, sizeof(original));  /* submit already copied */
    sBusy = 1u;
    assert(exampleTxService() == KYMO_BUSY && sWriteCalls == 0u);
    assert(exampleTxSubmit(original, sizeof(original)) == KYMO_BUSY);
    sBusy = 0u;
    ++calls;
    assert(exampleTxService() == KYMO_BUSY && sWriteCalls == calls);  /* zero write */
    ++calls;
    assert(exampleTxService() == KYMO_BUSY && sWriteCalls == calls);  /* first half */
    assert(exampleTxService() == KYMO_BUSY && sWriteCalls == calls);  /* borrowed */
    releaseTransfer();
    ++calls;
    assert(exampleTxService() == KYMO_BUSY && sWriteCalls == calls);  /* final half */
    assert(exampleTxSubmit(original, sizeof(original)) == KYMO_BUSY);
    assert(exampleTxService() == KYMO_BUSY && sWriteCalls == calls);
    releaseTransfer();
    assert(exampleTxService() == KYMO_IDLE && sWriteCalls == calls);
    assert(sWireLength == sizeof(EXPECTED_FRAME) &&
        memcmp(sWire, EXPECTED_FRAME, sizeof(EXPECTED_FRAME)) == 0);

    /* Exact maximum capacity works too, without truncation or duplicate writes. */
    assert(exampleTxSubmit(maximum, sizeof(maximum)) == KYMO_OK);
    for (size_t offset = 0u; offset < sizeof(maximum); offset += TRANSFER_CHUNK_BYTES) {
        assert(exampleTxService() == KYMO_BUSY);
        releaseTransfer();
    }
    assert(exampleTxService() == KYMO_IDLE && sWireLength == sizeof(sWire));
    assert(memcmp(sWire + sizeof(EXPECTED_FRAME), maximum, sizeof(maximum)) == 0);

    /* A contract violation is permanent, including subsequent submissions. */
    sViolate = 1u;
    assert(exampleTxSubmit(original, sizeof(original)) == KYMO_OK);
    assert(exampleTxService() == KYMO_IO_ERROR);
    calls = sWriteCalls;
    assert(exampleTxSubmit(original, sizeof(original)) == KYMO_IO_ERROR);
    assert(exampleTxService() == KYMO_IO_ERROR && sWriteCalls == calls);
    return 0;
}
