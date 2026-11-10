/* SPDX-License-Identifier: MIT */
/** @file tx_c_test.c
 * @brief C99 consumer linked against the C++11 transport implementation.
 */
#include <KymoCore.h>
#include <assert.h>
#include <string.h>

/*******************************************************************************
 * copyBytes
 ******************************************************************************/
static size_t copyBytes(void *user, const uint8_t *bytes, size_t length)
{
    memcpy(user, bytes, length);
    return length;
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    const uint8_t frame[] = { 1u, 2u, 3u };
    uint8_t storage[sizeof(frame)] = {0};
    uint8_t received[sizeof(frame)] = {0};
    KymoTx tx = {0};
    KymoTxConfig config = {0};
    config.buffer = storage;
    config.capacity = sizeof(storage);
    config.write = copyBytes;
    config.transport_user = received;
    assert(KymoTx_Init(&tx, &config) == KYMO_OK);
    assert(KymoTx_Submit(&tx, frame, sizeof(frame)) == KYMO_OK);
    assert(KymoTx_Service(&tx) == KYMO_IDLE);
    assert(memcmp(frame, received, sizeof(frame)) == 0);
    return 0;
}
