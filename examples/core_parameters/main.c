/* SPDX-License-Identifier: MIT */
/**
 * @file main.c
 * @brief Core and Parameter application using only their public interfaces.
 * @details The native callbacks simulate a PC command and a copying driver.
 * Replace them with nonblocking hardware callbacks on a device.
 */
#include <kymo_core.h>
#include <kymo_core_param.h>
#include <stdio.h>
#include <string.h>

/** @brief Initial value before the PC command arrives. */
#define EXAMPLE_INITIAL_SETPOINT 100.0f

/** @brief Value encoded by the documented golden command below. */
#define EXAMPLE_REQUESTED_SETPOINT 200.0f

/** @brief Largest value the application allows the PC to set. */
#define EXAMPLE_MAXIMUM_SETPOINT 300.0f

/** @brief Tolerance used only to check the native demonstration result. */
#define EXAMPLE_VALUE_TOLERANCE 0.001f

/** @brief Example wire identifier, RX budget and finite host-run limits. */
enum
{
    EXAMPLE_PARAMETER_ID = 42,
    EXAMPLE_RX_BUFFER_SIZE_BYTES = 8,
    EXAMPLE_MAIN_CALL_LIMIT = 32,
    EXAMPLE_REPLY_SIZE_BYTES = 9
};

static float sSetpoint = EXAMPLE_INITIAL_SETPOINT;
static const KymoParameter parameterTableAv[] = {
    KYMO_PARAM(EXAMPLE_PARAMETER_ID, sSetpoint, 0.0f, EXAMPLE_MAXIMUM_SETPOINT, "")};
static uint8_t txBufferAv[KYMO_PARAM_FRAME_MAX_SIZE];
static uint8_t rxBufferAv[EXAMPLE_RX_BUFFER_SIZE_BYTES];
static size_t sReadOffsetBytes;
static size_t sWrittenByteCount;

/**
 * @brief Native stand-in for the device's nonblocking receive callback.
 * @param[in,out] pContext Unused in this native demonstration.
 * @param[out] pBytesAv Receive array owned by Core.
 * @param[in] bufferSizeBytes Maximum bytes to copy without waiting.
 * @return Number of bytes actually read.
 */
static size_t CoreParametersExample_readBytes(void *pContext, uint8_t *pBytesAv,
                                              size_t bufferSizeBytes)
{
    /* Golden DATA frame: sync A5 5A, descriptor 40, ID 42, float 200, CRC8 F2. */
    static const uint8_t parameterCommandAv[] = {0xa5u, 0x5au, 0x40u, 0x2au, 0x00u,
                                                 0x00u, 0x48u, 0x43u, 0xf2u};
    size_t availableByteCount = sizeof(parameterCommandAv) - sReadOffsetBytes;
    (void)pContext;
    if (availableByteCount > bufferSizeBytes)
    {
        availableByteCount = bufferSizeBytes;
    }
    if (availableByteCount != 0u)
    {
        memcpy(pBytesAv, parameterCommandAv + sReadOffsetBytes, availableByteCount);
        sReadOffsetBytes += availableByteCount;
    }
    return availableByteCount;
}

/**
 * @brief Native stand-in for a device driver that copies output immediately.
 * @param[in,out] pContext Unused in this native demonstration.
 * @param[in] pBytesAv Read-only output array provided by Core.
 * @param[in] byteCount Number of offered bytes.
 * @return Accepted byte count; a real driver may accept fewer bytes or zero.
 */
static size_t CoreParametersExample_writeBytes(void *pContext, const uint8_t *pBytesAv,
                                               size_t byteCount)
{
    (void)pContext;
    (void)pBytesAv;
    sWrittenByteCount += byteCount;
    return byteCount;
}

/** @brief Fixed application resources and callbacks, stored outside Main. */
static const KymoCoreConfig coreConfig = {
    .readBytesCbk = CoreParametersExample_readBytes,
    .writeBytesCbk = CoreParametersExample_writeBytes,
    .pTxBufferAv = txBufferAv,
    .txBufferSizeBytes = sizeof(txBufferAv),
    .pRxBufferAv = rxBufferAv,
    .rxBufferSizeBytes = sizeof(rxBufferAv),
};

/** @brief Persistent parameter table used by the default Core. */
static const KymoCoreParamConfig parameterConfig = {
    .pParametersAv = parameterTableAv,
    .parameterCount = sizeof(parameterTableAv) / sizeof(parameterTableAv[0]),
};

/**
 * @brief Initialize Core and parameters, then use only the common Main call.
 * @return Zero after the verified command/readback demonstration, one on error.
 */
int main(void)
{
    KymoStatus status = KYMO_OK;
    int result = 1;

    status = KymoCore_init(&coreConfig);
    if (status == KYMO_OK)
    {
        status = KymoCoreParam_init(&parameterConfig);
    }
    for (uint8_t mainCallIndex = 0u;
         mainCallIndex < EXAMPLE_MAIN_CALL_LIMIT &&
         (status == KYMO_OK || status == KYMO_IDLE || status == KYMO_BUSY);
         ++mainCallIndex)
    {
        status = KymoCore_main();
    }
    if ((sSetpoint > EXAMPLE_REQUESTED_SETPOINT - EXAMPLE_VALUE_TOLERANCE) &&
        (sSetpoint < EXAMPLE_REQUESTED_SETPOINT + EXAMPLE_VALUE_TOLERANCE) &&
        (sWrittenByteCount == EXAMPLE_REPLY_SIZE_BYTES) && (status == KYMO_IDLE))
    {
        (void)puts("Parameter changed: 100 -> 200; readback: 9 bytes");
        result = 0;
    }
    return result;
}
