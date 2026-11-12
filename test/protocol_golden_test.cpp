/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "KymoCore.h"

class CaptureStream : public KymoStream
{
  public:
    uint8_t bytes[KYMO_FRAME_MAX_SIZE];
    size_t length;
    unsigned int writes;

    /***************************************************************************
     * CaptureStream
     **************************************************************************/
    CaptureStream() : length(0), writes(0)
    {
    }

    /***************************************************************************
     * write
     **************************************************************************/
    size_t write(const uint8_t *data, size_t data_length) override
    {
        assert(data_length <= sizeof(bytes));
        memcpy(bytes, data, data_length);
        length = data_length;
        ++writes;
        return data_length;
    }
};

/*******************************************************************************
 * main
 ******************************************************************************/
int main()
{
    static const uint8_t minimal_golden[] = {0xA5, 0x5A, KYMO_ENABLE_CRC ? 0x40 : 0x41,
                                             0x07, 0x00, 0x00,
                                             0x80, 0x3F, KYMO_ENABLE_CRC ? 0x54 : 0};
    static const uint8_t full_golden[] = {0xA5, 0x5A, KYMO_ENABLE_CRC ? 0x4E : 0x4F,
                                          0x03, 0x00, 0x00,
                                          0xA0, 0x3F, 0x00,
                                          0x00, 0x20, 0xC0,
                                          0x00, 0x00, 0x10,
                                          0x41, 0xD2, 0x04,
                                          0x00, 0x00, KYMO_ENABLE_CRC ? 0x89 : 0};
    static const uint8_t micro_golden[] = {
        0xA5, 0x5A, KYMO_ENABLE_CRC ? 0x52 : 0x53, 0x03, 0x00, 0x00, 0x20, 0xC0, 0x87, 0xD6,
        0x12, 0x00, KYMO_ENABLE_CRC ? 0x97 : 0};

    uint8_t frame[KYMO_FRAME_MAX_SIZE];
    KymoDataPoint point = {};
    KymoDataPoint decoded = {};

    point.id = 3;
    point.flags = KYMO_FLAG_X | KYMO_FLAG_Z | KYMO_FLAG_TIMESTAMP;
    point.x = 1.25f;
    point.value = -2.5f;
    point.z = 9.0f;
    point.timestamp_ms = 1234;

    size_t length = KymoProtocol_Encode(&point, frame, sizeof(frame));
    assert(length == sizeof(full_golden));
    assert(memcmp(frame, full_golden, sizeof(full_golden)) == 0);
    assert(KymoProtocol_Decode(frame, length, &decoded) == KYMO_OK);
    assert(decoded.id == point.id);
    assert(decoded.x == point.x);
    assert(decoded.value == point.value);
    assert(decoded.z == point.z);
    assert(decoded.timestamp_ms == point.timestamp_ms);

    KymoDataPoint micro = {};
    micro.id = 3;
    micro.flags = KYMO_FLAG_TIMESTAMP | KYMO_FLAG_TIMESTAMP_US;
    micro.value = -2.5f;
    micro.timestamp_ms = 1234567u;
    length = KymoProtocol_Encode(&micro, frame, sizeof(frame));
    assert(length == sizeof(micro_golden));
    assert(memcmp(frame, micro_golden, sizeof(micro_golden)) == 0);
    decoded = {};
    assert(KymoProtocol_Decode(frame, length, &decoded) == KYMO_OK);
    assert(decoded.flags == micro.flags);
    assert(decoded.timestamp_ms == micro.timestamp_ms);

    /* The unit bit without a timestamp field is rejected on both sides. */
    micro.flags = KYMO_FLAG_TIMESTAMP_US;
    assert(KymoProtocol_Encode(&micro, frame, sizeof(frame)) == 0u);
    assert(KymoProtocol_FrameLength(KYMO_DESCRIPTOR_DATA | KYMO_FLAG_TIMESTAMP_US) == 0u);
    assert(KymoProtocol_FrameLength(0x60u) == 0u);

    CaptureStream stream;
    KymoSender sender(stream);
    assert(sender.send(7, 1.0f) == KYMO_OK);
    assert(sender.flush() == KYMO_OK);
    assert(stream.writes == 1);
    assert(stream.length == sizeof(minimal_golden));
    assert(memcmp(stream.bytes, minimal_golden, sizeof(minimal_golden)) == 0);

    puts("Embedded protocol golden vectors passed");
    return 0;
}
