/* SPDX-License-Identifier: MIT */
/* KymoMemory: MEMORY_INFO frames, sampling rounds and configuration checks. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../src/features/memory/kymo_memory.h"
#include "../src/binary/embedded_crc.h"

static uint32_t values[3] = {3712u, 18432u, 14080u};
static uint8_t skip_index = 0xFFu;

/****************************************************************************************************
 * readValue
 ***************************************************************************************************/
static uint8_t readValue(void *context, size_t index, uint32_t *bytes)
{
    uint8_t ok = 0u;
    (void)context;
    if (index != skip_index)
    {
        *bytes = values[index];
        ok = 1u;
    }
    return ok;
}

/****************************************************************************************************
 * main
 ***************************************************************************************************/
int main(void)
{
    static const KymoMemoryEntry entries[3] = {
        {"parser", 4096u, KYMO_MEMORY_STACK_PEAK, 220u},
        {"heap", 32768u, KYMO_MEMORY_HEAP_FREE, 221u},
        {"a_name_longer_than_sixteen", 32768u, KYMO_MEMORY_HEAP_MIN_FREE, 222u},
    };
    KymoCoreMemoryConfig config = {readValue, nullptr, entries, 3u, 500u, 5000u};
    KymoMemory memory;
    KymoDataPoint point;
    uint8_t frame[KYMO_MEMORY_FRAME_MAX_SIZE];
    assert(KymoMemory_init(&memory, &config) == KYMO_OK);

    /* Golden vector: a5 5a 60 0d 07 dc 01 00 10 00 00 "parser" crc */
    size_t length = KymoMemory_nextInfo(&memory, 0u, frame, sizeof(frame));
    const uint8_t expected_head[] = {0xA5u, 0x5Au, 0x60u, 0x0Du, 0x07u, 0xDCu,
                                     0x01u, 0x00u, 0x10u, 0x00u, 0x00u};
    assert(length == 18u && memcmp(frame, expected_head, sizeof(expected_head)) == 0);
    assert(memcmp(frame + 11, "parser", 6u) == 0);
    assert(frame[17] == emb_crc8_msb(frame + 2, 15u, 0x07u, 0u));
    assert(KymoMemory_nextInfo(&memory, 0u, frame, sizeof(frame)) == 16u);

    /* The long name is cut to KYMO_MEMORY_NAME_MAX bytes. */
    assert(KymoMemory_nextInfo(&memory, 0u, frame, sizeof(frame)) == 12u + KYMO_MEMORY_NAME_MAX);
    assert(frame[3] == 7u + KYMO_MEMORY_NAME_MAX);
    assert(KymoMemory_nextInfo(&memory, 10u, frame, sizeof(frame)) == 0u);

    /* Too small a buffer sends nothing; the round repeats after info_period_ms. */
    assert(KymoMemory_nextInfo(&memory, 5000u, frame, 8u) == 0u);
    assert(KymoMemory_nextInfo(&memory, 5000u, frame, sizeof(frame)) == 18u);
    KymoMemory_requestInfo(&memory);
    assert(KymoMemory_nextInfo(&memory, 5001u, frame, sizeof(frame)) == 18u);

    /* First round at once, one entry per call, then idle until the period passes. */
    assert(KymoMemory_nextSample(&memory, 100u, &point) == KYMO_OK && point.id == 220u &&
           point.value == 3712.0f);
    assert(KymoMemory_nextSample(&memory, 100u, &point) == KYMO_OK && point.id == 221u);
    assert(KymoMemory_nextSample(&memory, 100u, &point) == KYMO_OK && point.id == 222u &&
           point.value == 14080.0f);
    assert(KymoMemory_nextSample(&memory, 599u, &point) == KYMO_IDLE);
    skip_index = 1u;
    assert(KymoMemory_nextSample(&memory, 600u, &point) == KYMO_OK && point.id == 220u);
    assert(KymoMemory_nextSample(&memory, 600u, &point) == KYMO_OK && point.id == 222u);
    assert(memory.read_failures == 1u);
    assert(KymoMemory_nextSample(&memory, 601u, &point) == KYMO_IDLE);

    /* Configuration checks. */
    KymoMemory other;
    KymoCoreMemoryConfig bad = config;
    bad.entryCount = 0u;
    assert(KymoMemory_init(&other, &bad) == KYMO_BAD_CONFIG);
    bad = config;
    bad.samplePeriodMs = 0u;
    assert(KymoMemory_init(&other, &bad) == KYMO_BAD_CONFIG);
    bad = config;
    bad.readBytesCbk = nullptr;
    assert(KymoMemory_init(&other, &bad) == KYMO_BAD_CONFIG);
    static const KymoMemoryEntry wrong_kind[1] = {{"x", 0u, 9u, 1u}};
    bad = config;
    bad.pEntriesAv = wrong_kind;
    bad.entryCount = 1u;
    assert(KymoMemory_init(&other, &bad) == KYMO_BAD_CONFIG);
    assert(KymoMemory_nextSample(nullptr, 0u, &point) == KYMO_BAD_CONFIG);
    return 0;
}
