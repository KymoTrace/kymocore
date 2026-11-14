/* SPDX-License-Identifier: MIT */
/**
 * @file core_entry_test.c
 * @brief C99 consumer exercising only the two new public headers.
 */
#include <kymo_core.h>
#include <kymo_core_param.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

/** @brief Fixture buffer sizes, fragmented I/O budget and bounded test duration. */
enum
{
    TEST_TX_BUFFER_SIZE_BYTES = 64,
    TEST_IO_FRAGMENT_SIZE_BYTES = 2,
    TEST_PARAMETER_FRAME_SIZE_BYTES = 9,
    TEST_CAPTURE_SIZE_BYTES = 32,
    TEST_MAIN_CALL_LIMIT = 40,
    TEST_PARAMETER_ID = 42,
    TEST_ARGUMENT_COUNT = 2,
    TEST_CASE_ARGUMENT_INDEX = 1
};

/** @brief Original value and limits used by the golden frame fixtures. */
#define TEST_INITIAL_VALUE 100.0f

/** @brief Accepted value encoded in the command fixture. */
#define TEST_REQUESTED_VALUE 200.0f

/** @brief Maximum accepted parameter value. */
#define TEST_MAXIMUM_VALUE 300.0f

/** @brief Tolerance for host-side verification of decoded floats. */
#define TEST_VALUE_TOLERANCE 0.001f

/** @brief Flip every CRC bit to exercise rejection. */
#define TEST_CORRUPT_CRC_MASK 0xffu
static uint8_t txBufferAv[TEST_TX_BUFFER_SIZE_BYTES];
static uint8_t rxBufferAv[TEST_IO_FRAGMENT_SIZE_BYTES];
static uint8_t receivedFrameAv[TEST_PARAMETER_FRAME_SIZE_BYTES];
static uint8_t transmittedBytesAv[TEST_CAPTURE_SIZE_BYTES];
static size_t inputOffset, outputLength, reads, writes;
static bool isReadOverflowEnabled = false;

/**
 * @brief Fragment incoming commands and optionally violate the reader contract.
 */
static size_t CoreEntryTest_readBytes(void *pContext, uint8_t *pBytesAv, size_t bufferSizeBytes)
{
    size_t count = sizeof(receivedFrameAv) - inputOffset;
    (void)pContext;
    ++reads;
    if (count > bufferSizeBytes)
    {
        count = bufferSizeBytes;
    }
    if (count != 0u)
    {
        memcpy(pBytesAv, receivedFrameAv + inputOffset, count);
        inputOffset += count;
    }
    if (isReadOverflowEnabled == true)
    {
        count = bufferSizeBytes + 1u;
    }
    return count;
}

/**
 * @brief Exercise partial transmission with two bytes accepted per call.
 */
static size_t CoreEntryTest_writeBytes(void *pContext, const uint8_t *pBytesAv, size_t byteCount)
{
    size_t count =
        byteCount > TEST_IO_FRAGMENT_SIZE_BYTES ? TEST_IO_FRAGMENT_SIZE_BYTES : byteCount;
    (void)pContext;
    ++writes;
    assert(outputLength + count <= sizeof(transmittedBytesAv));
    memcpy(transmittedBytesAv + outputLength, pBytesAv, count);
    outputLength += count;
    return count;
}

#if KYMO_EFFECTIVE_PARAMETERS

/**
 * @brief Check decoded values within the documented fixture tolerance.
 */
static bool CoreEntryTest_isValueEqual(float actualValue, float expectedValue)
{
    float difference = actualValue - expectedValue;
    bool isEqual = (difference > -TEST_VALUE_TOLERANCE) && (difference < TEST_VALUE_TOLERANCE);
    return isEqual;
}
#endif

/**
 * @brief Each invocation starts with fresh process-owned Core state.
 */
int main(int argc, char **argv)
{
    KymoCoreConfig core = {0};
    const uint8_t parameterRequestAv[] = {0xa5u, 0x5au, 0x40u, 0x2au, 0x00u,
                                          0x00u, 0x48u, 0x43u, 0xf2u};
#if KYMO_EFFECTIVE_PARAMETERS
    const uint8_t outOfRangeRequestAv[] = {0xa5u, 0x5au, 0x40u, 0x2au, 0x00u,
                                           0x00u, 0xfau, 0x43u, 0x97u};
    const uint8_t unchangedReplyAv[] = {0xa5u, 0x5au, 0x40u, 0x2au, 0x00u,
                                        0x00u, 0xc8u, 0x42u, 0x43u};
    static float target = TEST_INITIAL_VALUE;
    static const KymoParameter parameterTableAv[] = {
        KYMO_PARAM(TEST_PARAMETER_ID, target, 0.0f, TEST_MAXIMUM_VALUE, "")};
    KymoCoreParamConfig parameters = {parameterTableAv,
                                      sizeof(parameterTableAv) / sizeof(parameterTableAv[0])};
#endif
    assert(argc == TEST_ARGUMENT_COUNT);
    memcpy(receivedFrameAv, parameterRequestAv, sizeof(parameterRequestAv));
    core.pTxBufferAv = txBufferAv;
    core.txBufferSizeBytes = sizeof(txBufferAv);
    core.writeBytesCbk = CoreEntryTest_writeBytes;
    core.readBytesCbk = CoreEntryTest_readBytes;
    core.pRxBufferAv = rxBufferAv;
    core.rxBufferSizeBytes = sizeof(rxBufferAv);
    assert(KymoCore_main() == KYMO_BAD_CONFIG);
    if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "empty") == 0)
    {
        assert(KymoCore_init(NULL) == KYMO_OK);
        assert(KymoCore_init(NULL) == KYMO_REJECTED);
        assert(KymoCore_main() == KYMO_IDLE);
        assert(KymoCore_main() == KYMO_IDLE);
        assert(reads == 0u && writes == 0u);
    }
    else
    {
#if KYMO_EFFECTIVE_PARAMETERS
        if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "before-init") == 0)
        {
            assert(KymoCoreParam_init(&parameters) == KYMO_BAD_CONFIG);
        }
#endif
        if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "retry") == 0)
        {
            core.writeBytesCbk = NULL;
            assert(KymoCore_init(&core) == KYMO_BAD_CONFIG);
            core.writeBytesCbk = CoreEntryTest_writeBytes;
        }
        if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "small") == 0)
        {
            core.txBufferSizeBytes = 1u;
        }
        assert(KymoCore_init(&core) == KYMO_OK);
        assert(KymoCore_init(&core) == KYMO_REJECTED);
        if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "idle") == 0 ||
            strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "retry") == 0)
        {
            assert(KymoCore_main() == KYMO_IDLE);
            assert(reads == 0u && writes == 0u);
        }
#if KYMO_EFFECTIVE_PARAMETERS
        else if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "late") == 0)
        {
            assert(KymoCore_main() == KYMO_IDLE);
            assert(KymoCoreParam_init(&parameters) == KYMO_REJECTED);
            assert(KymoCore_main() == KYMO_IDLE);
        }
        else if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "invalid") == 0 ||
                 strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "small") == 0 ||
                 strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "null") == 0)
        {
            if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "invalid") == 0)
            {
                parameters.parameterCount = 0u;
            }
            assert(KymoCoreParam_init(strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "null") == 0
                                          ? NULL
                                          : &parameters) == KYMO_BAD_CONFIG);
            assert(KymoCore_main() == KYMO_BAD_CONFIG);
            assert(reads == 0u && writes == 0u &&
                   CoreEntryTest_isValueEqual(target, TEST_INITIAL_VALUE) == true);
        }
        else
        {
            assert(KymoCoreParam_init(&parameters) == KYMO_OK);
            assert(reads == 0u && writes == 0u &&
                   CoreEntryTest_isValueEqual(target, TEST_INITIAL_VALUE) == true);
            if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "duplicate") == 0)
            {
                assert(KymoCoreParam_init(&parameters) == KYMO_REJECTED);
                assert(KymoCore_main() == KYMO_REJECTED);
                assert(reads == 0u && writes == 0u);
            }
            else if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "overflow") == 0)
            {
                isReadOverflowEnabled = true;
                assert(KymoCore_main() == KYMO_IO_ERROR);
                assert(KymoCore_main() == KYMO_IO_ERROR);
            }
            else
            {
                if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "crc") == 0)
                {
                    receivedFrameAv[sizeof(receivedFrameAv) - 1u] ^= TEST_CORRUPT_CRC_MASK;
                }
                if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "range") == 0)
                {
                    memcpy(receivedFrameAv, outOfRangeRequestAv, sizeof(receivedFrameAv));
                }
                for (uint8_t turn = 0u; turn < TEST_MAIN_CALL_LIMIT; ++turn)
                {
                    KymoStatus status = KymoCore_main();
                    assert(status == KYMO_OK || status == KYMO_BUSY || status == KYMO_IDLE);
                }
                if (strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "crc") == 0)
                {
                    assert(CoreEntryTest_isValueEqual(target, TEST_INITIAL_VALUE) == true &&
                           outputLength == 0u);
                }
                else
                {
                    const uint8_t *pExpectedReplyAv =
                        strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "range") == 0 ? unchangedReplyAv
                                                                             : parameterRequestAv;
                    assert(CoreEntryTest_isValueEqual(
                               target, strcmp(argv[TEST_CASE_ARGUMENT_INDEX], "range") == 0
                                           ? TEST_INITIAL_VALUE
                                           : TEST_REQUESTED_VALUE) == true);
                    assert(outputLength == sizeof(parameterRequestAv));
                    assert(memcmp(transmittedBytesAv, pExpectedReplyAv,
                                  sizeof(parameterRequestAv)) == 0);
                }
                assert(KymoCoreParam_init(&parameters) == KYMO_REJECTED);
                assert(KymoCore_main() == KYMO_IDLE);
            }
        }
#endif
    }
    return 0;
}
