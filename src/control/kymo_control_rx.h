/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_control_rx.h
 * @brief Internal: reassemble host frames for the back-channel modules.
 *
 * @details Shared by KymoParam and KymoWatch so both accept the same byte stream:
 * DATA frames (length from the descriptor) and control frames (descriptor
 * KYMO_DESCRIPTOR_PARAM, payload length 1..KYMO_CONTROL_REQUEST_MAX). Constant
 * work per byte, no heap. Not part of the public API; applications use the
 * modules instead.
 * @par Usage
 * Internal to src/control; include KymoCore.h in application code.
 */
#ifndef KYMO_CONTROL_RX_H
/**
 * @brief Include guard for kymo_control_rx.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * The compiler manages this guard.
 */
#define KYMO_CONTROL_RX_H

#include <stddef.h>
#include <stdint.h>
#include "../kymo_config.h"
#include "../telemetry/kymo_protocol.h"

#if KYMO_EFFECTIVE_PARAMETERS || KYMO_EFFECTIVE_WATCH || defined(DOXYGEN)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Largest payload of a host control request in bytes.
 * @details WATCH_SET needs 10 bytes; longer requests are discarded as invalid.
 * Five bytes of framing plus this limit fit into KYMO_FRAME_MAX_SIZE.
 * @par Usage
 * Internal bound of KymoControlRx.
 */
#define KYMO_CONTROL_REQUEST_MAX 16u

/**
 * @brief Control operation codes of every back-channel module.
 * @details A module handles its own operations and ignores the others, so one
 * byte stream can be fed to several modules. Values follow PROTOCOL.md.
 * @par Usage
 * KymoControl_IsKnownOperation(frame[4]) before counting a frame as invalid.
 */
#define KYMO_CONTROL_OP_FIRST 0x01u
/** @brief Highest operation code defined in PROTOCOL.md. */
#define KYMO_CONTROL_OP_LAST 0x06u

/**
 * @brief Reassembly state for one receiving module.
 *
 * @details frame holds the bytes of the current candidate; length counts them and
 * expected is the complete length once known (zero while unknown).
 * @par Usage
 * Embedded in KymoParamReceiver and KymoWatch; zero-initialize.
 */
typedef struct {
    /** @brief Bytes of the frame being reassembled. */
    uint8_t frame[KYMO_FRAME_MAX_SIZE];
    /** @brief Number of valid bytes in frame. */
    size_t length;
    /** @brief Complete-frame length once known, else zero. */
    size_t expected;
} KymoControlRx;

/**
 * @brief Consume one byte.
 *
 * @details Skips bytes until a sync marker; rejects unsupported descriptors and
 * control lengths outside 1..KYMO_CONTROL_REQUEST_MAX by incrementing *invalid.
 * When the function returns a length, rx->frame holds a complete frame of that
 * many bytes (CRC not yet checked); handle it before feeding the next byte, which
 * starts a new frame.
 * @param[in,out] rx Reassembly state; must not be NULL.
 * @param[in] byte Received byte.
 * @param[in,out] invalid Diagnostic counter of discarded frames; must not be NULL.
 * @return Length of the completed frame, or zero.
 * @par Usage
 * if ((n = KymoControlRx_Byte(&rx, byte, &invalid)) != 0u) { handle(rx.frame, n); }
 */
size_t KymoControlRx_Byte(KymoControlRx *rx, uint8_t byte, uint32_t *invalid);

/**
 * @brief Check the mandatory CRC of a complete control frame.
 * @param[in] frame Complete control frame; must not be NULL.
 * @param[in] length Its length (payload + 5).
 * @return 1 when the CRC-8 over descriptor, length and payload matches.
 * @par Usage
 * Call before interpreting the operation byte frame[4].
 */
uint8_t KymoControlRx_CrcValid(const uint8_t *frame, size_t length);

/**
 * @brief Whether an operation code belongs to any back-channel module.
 * @param[in] operation Operation byte (frame[4]).
 * @return 1 for codes KYMO_CONTROL_OP_FIRST..KYMO_CONTROL_OP_LAST.
 * @par Usage
 * Ignore known foreign operations; count unknown ones as invalid.
 */
uint8_t KymoControl_IsKnownOperation(uint8_t operation);

#ifdef __cplusplus
}
#endif

#endif /* KYMO_EFFECTIVE_PARAMETERS || KYMO_EFFECTIVE_WATCH */
#endif /* KYMO_CONTROL_RX_H */
