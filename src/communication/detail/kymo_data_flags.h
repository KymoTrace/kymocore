/* SPDX-License-Identifier: MIT */
/** @file kymo_data_flags.h
 * @brief Shared data field selectors, without encoding or transport interfaces.
 */
#ifndef KYMO_DATA_FLAGS_H
#define KYMO_DATA_FLAGS_H
#include "../../kymo_types.h"
#include "../../binary/embedded_bits.h"

/**
 * @brief Descriptor bit indicating a four-byte X coordinate.
 *
 * @details Adds one finite binary32 value before the mandatory Y/value field.
 * @par Usage
 * Set in KymoChannel::flags or KymoDataPoint::flags for XY or XYZ measurements.
 */
#define KYMO_FLAG_X 0x08u

/**
 * @brief Descriptor bit indicating a four-byte Z coordinate.
 *
 * @details Adds a finite binary32 value after Y. Z can also be used without X.
 * @par Usage
 * Combine with KYMO_FLAG_X for three-dimensional measurements.
 */
#define KYMO_FLAG_Z 0x04u

/**
 * @brief Descriptor bit indicating relative uint32 milliseconds.
 *
 * @details Adds four bytes after all coordinates. The counter wraps modulo 2^32; the app converts it to seconds.
 * @par Usage
 * Set on a channel to include time relative to KymoRuntime_init.
 */
#define KYMO_FLAG_TIMESTAMP 0x02u

/**
 * @brief Descriptor bit selecting microseconds for the timestamp field.
 *
 * @details Occupies message-type bit 4 (data type 01). Valid only together with
 * KYMO_FLAG_TIMESTAMP; the frame layout and length are unchanged, but
 * KymoDataPoint::timestamp_ms then carries uint32 microseconds (wraps after
 * about 71.6 minutes). Not part of KYMO_ALLOWED_FLAGS: the channel runtimes
 * supply millisecond clocks and reject it. Use it with KymoProtocol_encode only.
 * @par Usage
 * point.flags = KYMO_FLAG_TIMESTAMP | KYMO_FLAG_TIMESTAMP_US;
 * point.timestamp_ms = micros_since_epoch;
 */
#define KYMO_FLAG_TIMESTAMP_US 0x10u

/**
 * @brief Mask of every supported optional payload field.
 *
 * @details Only X, Z and timestamp flags are permitted on the wire. Build-time
 * feature switches further restrict usable combinations through KYMO_SUPPORTED_FLAGS.
 * @par Usage
 * Use for XYZ with timestamps or to validate a supplied flag byte.
 */
#define KYMO_ALLOWED_FLAGS EMB_U8_OR(KYMO_FLAG_X, EMB_U8_OR(KYMO_FLAG_Z, KYMO_FLAG_TIMESTAMP))

/**
 * @brief Payload flags compiled into this library build.
 * @details Wire constants remain unchanged; requests outside this mask fail.
 * Zero selects the default scalar-only build. No fields are silently removed.
 * @par Usage
 * Validate application channel flags against KYMO_SUPPORTED_FLAGS.
 */
#define KYMO_SUPPORTED_FLAGS                                \
    EMB_U8_OR((KYMO_ENABLE_X ? KYMO_FLAG_X : 0u),           \
              EMB_U8_OR((KYMO_ENABLE_Z ? KYMO_FLAG_Z : 0u), \
                        (KYMO_ENABLE_TIMESTAMP ? KYMO_FLAG_TIMESTAMP : 0u)))

/**
 * @brief Flags accepted by the raw codec of this library build.
 * @details KYMO_SUPPORTED_FLAGS plus KYMO_FLAG_TIMESTAMP_US when timestamps are enabled.
 * @par Usage
 * Validate KymoDataPoint::flags before KymoProtocol_encode.
 */
#define KYMO_CODEC_FLAGS \
    EMB_U8_OR(KYMO_SUPPORTED_FLAGS, (KYMO_ENABLE_TIMESTAMP ? KYMO_FLAG_TIMESTAMP_US : 0u))

/**
 * @brief Size in bytes of a Y-only frame.
 *
 * @details Two sync bytes, descriptor, channel, binary32 Y and CRC/zero trailer.
 * @par Usage
 * Minimum complete-frame bound for decoding.
 */
#define KYMO_FRAME_MIN_SIZE 9u

/**
 * @brief Maximum complete-frame size in bytes.
 *
 * @details Includes X, Y, Z, timestamp and framing. A single buffer of this size supports all flag combinations.
 * @par Usage
 * Declare uint8_t frame[KYMO_FRAME_MAX_SIZE].
 */
#define KYMO_FRAME_MAX_SIZE 21u
#endif
