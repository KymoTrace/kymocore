/* SPDX-License-Identifier: MIT */
/** @file kymo_port_native.cpp
 * @brief Deterministic file-backed port with short writes and simulated DMA.
 */
#include "kymo_port.h"
#include "kymo_port_native.h"
#include "kymo_user_config.h"
#include <stdio.h>
#include <string.h>

/** @brief Maximum bytes accepted per write; test override. */
#ifndef EXAMPLE_NATIVE_CHUNK
#define EXAMPLE_NATIVE_CHUNK 7u
#endif
/** @brief Cycles retaining the caller's pointer; zero copies synchronously. */
#ifndef EXAMPLE_NATIVE_HOLD
#define EXAMPLE_NATIVE_HOLD 2u
#endif
/** @brief Initial cycles with an unavailable transport; test override. */
#ifndef EXAMPLE_NATIVE_STALL
#define EXAMPLE_NATIVE_STALL 100u
#endif
/** @brief Inject one invalid accepted count to exercise fail-stop handling. */
#ifndef EXAMPLE_NATIVE_FAULT
#define EXAMPLE_NATIVE_FAULT 0
#endif

static const size_t RX_FRAGMENT_BYTES = 3u;
static const uint32_t ZERO_WRITE_INTERVAL = 5u;

static FILE *sInput = nullptr;
static FILE *sOutput = nullptr;
static uint64_t sTicks = 0u;
static const uint8_t *sBorrowed = nullptr;
static size_t sBorrowedLength = 0u;
static uint8_t sSaved[EXAMPLE_TX_CAPACITY];
static uint32_t sHoldCycles = 0u;
static uint32_t sWrites = 0u;
static uint8_t sHealthy = 0u;

/*******************************************************************************
 * exampleNativeOpen
 ******************************************************************************/
uint8_t exampleNativeOpen(const char *inputPath, const char *outputPath)
{
    uint8_t result = 0u;
    if ((inputPath != nullptr) && (outputPath != nullptr) && (strcmp(inputPath, outputPath) != 0))
    {
        sInput = fopen(inputPath, "rb");
        if (sInput != nullptr)
        {
            sOutput = fopen(outputPath, "wb");
            if (sOutput != nullptr)
            {
                sHealthy = 1u;
                result = 1u;
            }
            else
            {
                (void)fclose(sInput);
                sInput = nullptr;
            }
        }
    }
    return result;
}

/*******************************************************************************
 * examplePortService
 ******************************************************************************/
void examplePortService(void)
{
    ++sTicks;
    if (sHoldCycles != 0u)
    {
        --sHoldCycles;
    }
    if ((sBorrowed != nullptr) && (sHoldCycles == 0u))
    {
        /* Detect buffer reuse before simulated DMA completion. */
        if ((memcmp(sSaved, sBorrowed, sBorrowedLength) != 0) ||
            (fwrite(sBorrowed, 1u, sBorrowedLength, sOutput) != sBorrowedLength))
        {
            sHealthy = 0u;
        }
        sBorrowed = nullptr;
        sBorrowedLength = 0u;
    }
}

/*******************************************************************************
 * examplePortRead
 ******************************************************************************/
size_t examplePortRead(uint8_t *output, size_t capacity)
{
    size_t length = 0u;
    if ((output != nullptr) && (sInput != nullptr) && (sHealthy != 0u))
    {
        if (capacity > RX_FRAGMENT_BYTES)
        {
            capacity = RX_FRAGMENT_BYTES;
        }
        length = fread(output, 1u, capacity, sInput);
        if (ferror(sInput) != 0)
        {
            sHealthy = 0u;
        }
    }
    return length;
}

/*******************************************************************************
 * examplePortWrite
 ******************************************************************************/
size_t examplePortWrite(const uint8_t *bytes, size_t length)
{
    size_t accepted = 0u;
    if ((bytes != nullptr) && (length != 0u) && (sHealthy != 0u) && (examplePortBusy() == 0u))
    {
        ++sWrites;
#if EXAMPLE_NATIVE_FAULT
        /* A second write after the violation is itself an example failure. */
        sHealthy = (uint8_t)(sWrites == 1u);
        accepted = length + 1u;
#else
        /* Zero acceptance is legal even after busy returned zero. */
        if ((sWrites % ZERO_WRITE_INTERVAL) != 0u)
        {
            accepted = length;
            if (accepted > EXAMPLE_NATIVE_CHUNK)
            {
                accepted = EXAMPLE_NATIVE_CHUNK;
            }
            if (accepted > sizeof(sSaved))
            {
                accepted = sizeof(sSaved);
            }
            if (EXAMPLE_NATIVE_HOLD == 0u)
            {
                if (fwrite(bytes, 1u, accepted, sOutput) != accepted)
                {
                    sHealthy = 0u;
                }
            }
            else
            {
                sBorrowed = bytes;
                sBorrowedLength = accepted;
                memcpy(sSaved, bytes, accepted);
                sHoldCycles = EXAMPLE_NATIVE_HOLD;
            }
        }
#endif
    }
    return accepted;
}

/*******************************************************************************
 * examplePortBusy
 ******************************************************************************/
uint8_t examplePortBusy(void)
{
    uint8_t busy = (uint8_t)((sBorrowed != nullptr) || (sTicks < EXAMPLE_NATIVE_STALL));
    return busy;
}

/*******************************************************************************
 * examplePortClockMs
 ******************************************************************************/
uint32_t examplePortClockMs(void *user)
{
    (void)user;
    return (uint32_t)sTicks;
}

/*******************************************************************************
 * examplePortClockTicks
 ******************************************************************************/
uint64_t examplePortClockTicks(void *user)
{
    (void)user;
    return sTicks;
}

/*******************************************************************************
 * examplePortHealthy
 ******************************************************************************/
uint8_t examplePortHealthy(void)
{
    return sHealthy;
}

/*******************************************************************************
 * exampleNativeClose
 ******************************************************************************/
uint8_t exampleNativeClose(void)
{
    uint8_t result = (uint8_t)((sBorrowed == nullptr) && (sHealthy != 0u));
    if (sBorrowed == nullptr)
    {
        if ((sInput != nullptr) && (fclose(sInput) != 0))
        {
            result = 0u;
        }
        if ((sOutput != nullptr) && (fclose(sOutput) != 0))
        {
            result = 0u;
        }
        sInput = nullptr;
        sOutput = nullptr;
    }
    return result;
}
