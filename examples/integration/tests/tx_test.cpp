/* SPDX-License-Identifier: MIT */
/** @file tx_test.cpp
 * @brief Sender contract tests with an independently controlled borrowing port.
 */
#include <KymoCore.h>
#include <assert.h>
#include <string.h>

/* Explicit fixture bytes, split into two physical transfers. */
static const uint8_t EXPECTED_FRAME[] = {1u, 2u, 3u, 4u};
static const size_t TRANSFER_CHUNK_BYTES = 2u;
static const uint8_t REUSED_BUFFER_BYTE = 99u;

static const size_t TEST_CAPACITY = 300u;
static KymoTx sTx;
static uint8_t sBuffer[TEST_CAPACITY + 2u];
static const uint8_t GUARD_BYTE = 0xadu;
static uint8_t sBusy = 0u;
static uint8_t sRejectOnce = 1u;
static uint8_t sViolate = 0u;
static size_t sWriteCalls = 0u;
static const uint8_t *sBorrowed = nullptr;
static size_t sBorrowedLength = 0u;
static uint8_t sWire[TEST_CAPACITY + sizeof(EXPECTED_FRAME)];
static size_t sWireLength = 0u;

/*******************************************************************************
 * examplePortBusy
 ******************************************************************************/
static uint8_t examplePortBusy(void *user)
{
    assert(user == &sBusy);
    return sBusy;
}

/*******************************************************************************
 * examplePortWrite
 ******************************************************************************/
static size_t examplePortWrite(void *user, const uint8_t *bytes, size_t length)
{
    size_t accepted = 0u;
    assert(bytes != nullptr && length != 0u && sBusy == 0u);
    assert(user == &sBusy);
    ++sWriteCalls;
    if (sViolate != 0u)
    {
        accepted = length + 1u;
    }
    else if (sRejectOnce != 0u)
    {
        sRejectOnce = 0u;
    }
    else
    {
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
 * copyWrite
 ******************************************************************************/
static size_t copyWrite(void *user, const uint8_t *bytes, size_t length)
{
    size_t *calls = static_cast<size_t *>(user);
    assert(length == TEST_CAPACITY);
    for (size_t index = 0u; index < length; ++index)
    {
        assert(bytes[index] == REUSED_BUFFER_BYTE);
    }
    ++*calls;
    return length;
}

/*******************************************************************************
 * testIndependentSender
 ******************************************************************************/
static void testIndependentSender(void)
{
    KymoTx other = {};
    uint8_t buffer[TEST_CAPACITY] = {};
    size_t calls = 0u;
    KymoTxConfig config = {};
    config.buffer = buffer;
    config.capacity = sizeof(buffer);
    config.write = copyWrite;
    config.transport_user = &calls;
    memset(buffer, REUSED_BUFFER_BYTE, sizeof(buffer));
    assert(KymoTx_Init(&other, &config) == KYMO_OK);
    config.write = nullptr; /* Init copied the configuration. */
    /* Aliased input is supported; the size_t callback must handle >255 bytes. */
    assert(KymoTx_Submit(&other, buffer, sizeof(buffer)) == KYMO_OK);
    assert(KymoTx_Service(&other) == KYMO_IDLE);
    assert(calls == 1u);
    assert(KymoTx_Service(&other) == KYMO_IDLE && calls == 1u);
    /* The first sender is still borrowing its unchanged first half. */
    assert(sBusy != 0u && sBorrowedLength == TRANSFER_CHUNK_BYTES);
    assert(memcmp(sBorrowed, EXPECTED_FRAME, sBorrowedLength) == 0);
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    uint8_t original[sizeof(EXPECTED_FRAME)] = {};
    uint8_t maximum[TEST_CAPACITY] = {};
    size_t calls = 0u;
    KymoTxConfig config = {};
    config.buffer = sBuffer + 1u;
    config.capacity = TEST_CAPACITY;
    config.write = examplePortWrite;
    config.busy = examplePortBusy;
    config.transport_user = &sBusy;
    sBuffer[0] = GUARD_BYTE;
    sBuffer[sizeof(sBuffer) - 1u] = GUARD_BYTE;
    assert(KymoTx_Init(nullptr, &config) == KYMO_BAD_CONFIG);
    assert(KymoTx_Init(&sTx, nullptr) == KYMO_BAD_CONFIG);
    assert(KymoTx_Service(nullptr) == KYMO_BAD_CONFIG);
    assert(KymoTx_Service(&sTx) == KYMO_BAD_CONFIG);
    assert(KymoTx_Submit(nullptr, original, sizeof(original)) == KYMO_BAD_CONFIG);
    assert(KymoTx_Submit(&sTx, original, sizeof(original)) == KYMO_BAD_CONFIG);
    config.buffer = nullptr;
    assert(KymoTx_Init(&sTx, &config) == KYMO_BAD_CONFIG);
    config.buffer = sBuffer + 1u;
    config.capacity = 0u;
    assert(KymoTx_Init(&sTx, &config) == KYMO_BAD_CONFIG);
    config.capacity = TEST_CAPACITY;
    config.write = nullptr;
    assert(KymoTx_Init(&sTx, &config) == KYMO_BAD_CONFIG);
    config.write = examplePortWrite;
    assert(KymoTx_Init(&sTx, &config) == KYMO_OK);
    assert(KymoTx_Init(&sTx, &config) == KYMO_REJECTED);
    memcpy(original, EXPECTED_FRAME, sizeof(original));
    assert(KymoTx_Service(&sTx) == KYMO_IDLE);
    assert(KymoTx_Submit(&sTx, nullptr, sizeof(original)) == KYMO_BAD_CONFIG);
    assert(KymoTx_Submit(&sTx, original, 0u) == KYMO_BAD_CONFIG);
    assert(KymoTx_Submit(&sTx, original, TEST_CAPACITY + 1u) == KYMO_BAD_CONFIG);
    assert(KymoTx_Submit(&sTx, original, sizeof(original)) == KYMO_OK);
    memset(original, REUSED_BUFFER_BYTE, sizeof(original)); /* submit already copied */
    sBusy = 1u;
    assert(KymoTx_Service(&sTx) == KYMO_BUSY && sWriteCalls == 0u);
    assert(KymoTx_Submit(&sTx, original, sizeof(original)) == KYMO_BUSY);
    sBusy = 0u;
    ++calls;
    assert(KymoTx_Service(&sTx) == KYMO_BUSY && sWriteCalls == calls); /* zero write */
    ++calls;
    assert(KymoTx_Service(&sTx) == KYMO_BUSY && sWriteCalls == calls); /* first half */
    assert(KymoTx_Service(&sTx) == KYMO_BUSY && sWriteCalls == calls); /* borrowed */
    assert(KymoTx_Init(&sTx, &config) == KYMO_REJECTED);
    testIndependentSender();
    releaseTransfer();
    ++calls;
    assert(KymoTx_Service(&sTx) == KYMO_BUSY && sWriteCalls == calls); /* final half */
    assert(KymoTx_Submit(&sTx, original, sizeof(original)) == KYMO_BUSY);
    assert(KymoTx_Service(&sTx) == KYMO_BUSY && sWriteCalls == calls);
    releaseTransfer();
    assert(KymoTx_Service(&sTx) == KYMO_IDLE && sWriteCalls == calls);
    assert(sWireLength == sizeof(EXPECTED_FRAME) &&
           memcmp(sWire, EXPECTED_FRAME, sizeof(EXPECTED_FRAME)) == 0);

    /* Exact maximum capacity works too, without truncation or duplicate writes. */
    assert(KymoTx_Submit(&sTx, maximum, sizeof(maximum)) == KYMO_OK);
    for (size_t offset = 0u; offset < sizeof(maximum); offset += TRANSFER_CHUNK_BYTES)
    {
        assert(KymoTx_Service(&sTx) == KYMO_BUSY);
        releaseTransfer();
    }
    assert(KymoTx_Service(&sTx) == KYMO_IDLE && sWireLength == sizeof(sWire));
    assert(memcmp(sWire + sizeof(EXPECTED_FRAME), maximum, sizeof(maximum)) == 0);

    assert(sBuffer[0] == GUARD_BYTE && sBuffer[sizeof(sBuffer) - 1u] == GUARD_BYTE);

    /* A contract violation is permanent, including subsequent submissions. */
    sViolate = 1u;
    assert(KymoTx_Submit(&sTx, original, sizeof(original)) == KYMO_OK);
    assert(KymoTx_Service(&sTx) == KYMO_IO_ERROR);
    calls = sWriteCalls;
    assert(KymoTx_Init(&sTx, &config) == KYMO_REJECTED);
    assert(KymoTx_Submit(&sTx, original, sizeof(original)) == KYMO_IO_ERROR);
    assert(KymoTx_Service(&sTx) == KYMO_IO_ERROR && sWriteCalls == calls);
    return 0;
}
