/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file kymo_param.h
 * @defgroup kymo_param Back channel: host writes to parameters
 * @{
 * @brief Let the host change registered application parameters at run time.
 *
 * @details The host sends an ordinary Kymo DATA frame in the opposite direction:
 * the channel ID names the parameter, the value is the new setting. The receiver
 * reassembles frames from raw received bytes, and applies a write only when
 *
 * - the frame is complete, CRC-protected (NO_CRC frames are discarded) and has
 *   no X, Z or timestamp field,
 * - the ID belongs to a registered parameter, and
 * - the finite value lies within the parameter's [minimum, maximum].
 *
 * Every accepted write and every out-of-range write marks the parameter for a
 * readback. The application sends the readback as a normal DATA frame with the
 * same ID and the value the parameter now holds; the host confirms its write by
 * that readback. An out-of-range write therefore reports the unchanged value.
 * Writes with an unknown ID, a bad CRC or a bad layout get no readback.
 *
 * Discovery: the host asks for the parameter list with a parameter control frame
 * (KYMO_DESCRIPTOR_PARAM, operation KYMO_PARAM_OP_LIST). The receiver then queues
 * one description per parameter (ID, name, unit, limits) and a readback of every
 * current value. KymoParam_NextDescription encodes the descriptions; the host
 * fills its parameter panel from them, so names and IDs live only in this table.
 *
 * Cost: KymoParam_Feed does constant work per byte and one table lookup of at
 * most KYMO_MAX_PARAMETERS entries per complete frame. Nothing runs unless the
 * application feeds bytes; nothing is compiled unless KYMO_ENABLE_PARAMETERS=1.
 * No heap, no callbacks, no SDK access: the application reads its transport.
 *
 * Concurrency: the receiver is not thread-safe. Call Feed and NextReadback from
 * one task. A parameter is a float; a 32-bit aligned float store is single-copy
 * atomic on 32-bit MCUs, so another task may read it without a lock. On 8-bit
 * MCUs read it in the same task or with interrupts disabled.
 *
 * The table maps an ID to an address with limits. A later firmware-image (ELF)
 * based mode can produce the same kind of entries from symbol addresses; the
 * range and readback rules above stay the contract.
 * @par Usage
 * @code{.c}
 * static float kp = 1.0f;
 * static const KymoParameter params[] = { KYMO_PARAM(100, kp, 0.0f, 10.0f, "") };
 * static KymoParamReceiver receiver;
 * KymoParam_Init(&receiver, params, 1u);
 * // In the application loop, with bytes read without blocking:
 * KymoParam_Feed(&receiver, rx, rx_length);
 * uint8_t frame[KYMO_PARAM_FRAME_MAX_SIZE];
 * size_t length;
 * while ((length = KymoParam_NextDescription(&receiver, frame, sizeof(frame))) != 0u) { send(frame, length); }
 * KymoDataPoint readback;
 * while (KymoParam_NextReadback(&receiver, &readback) == KYMO_OK) { send(readback); }
 * @endcode
 */
#ifndef KYMO_PARAM_H
/**
 * @brief Include guard for kymo_param.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include KymoCore.h normally; the compiler manages this guard.
 */
#define KYMO_PARAM_H

#include <stddef.h>
#include <stdint.h>
#include "../kymo_types.h"
#include "../telemetry/kymo_protocol.h"
#include "kymo_control_rx.h"

#if KYMO_EFFECTIVE_PARAMETERS || defined(DOXYGEN)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parameter control operation: host asks for the parameter list.
 * @details Payload is this single byte. Unknown operations are discarded.
 * @par Usage
 * Sent by the host; the receiver handles it inside KymoParam_Feed.
 */
#define KYMO_PARAM_OP_LIST 0x01u

/**
 * @brief Parameter control operation: device describes one parameter.
 * @details Payload: op, id, index, count, minimum (f32 LE), maximum (f32 LE),
 * name length, name bytes, unit length, unit bytes. Names and units are UTF-8
 * without terminator; index and count let the host see when the list is complete.
 * @par Usage
 * Produced by KymoParam_NextDescription.
 */
#define KYMO_PARAM_OP_DESCRIPTION 0x02u

/**
 * @brief Size of a compact write: parameter ID (u8) and value (f32 LE).
 * @details For links whose own framing and checksum carry the message, such as a
 * classic CAN data field of up to 8 bytes. See KymoParam_FeedCompact.
 * @par Usage
 * Check the received data length against it before calling KymoParam_FeedCompact.
 */
#define KYMO_PARAM_COMPACT_SIZE 5u

/**
 * @brief Longest parameter name in bytes, without terminator.
 * @details KymoParam_Init rejects longer names.
 * @par Usage
 * Keep variable names used with KYMO_PARAM within this length.
 */
#define KYMO_PARAM_NAME_MAX 24u

/**
 * @brief Longest unit text in bytes, without terminator.
 * @details KymoParam_Init rejects longer units.
 * @par Usage
 * Short units such as "°C" or "rpm".
 */
#define KYMO_PARAM_UNIT_MAX 8u

/**
 * @brief Largest parameter control frame: one description with maximum name and unit.
 * @details Sync (2), descriptor, length, 14 fixed payload bytes, name, unit and CRC.
 * @par Usage
 * Declare uint8_t frame[KYMO_PARAM_FRAME_MAX_SIZE] for KymoParam_NextDescription.
 */
#define KYMO_PARAM_FRAME_MAX_SIZE (19u + KYMO_PARAM_NAME_MAX + KYMO_PARAM_UNIT_MAX)

/**
 * @brief Table entry for a variable; the variable's name becomes the parameter name.
 *
 * @details Expands to a KymoParameter initializer. Each argument is used once;
 * the name is the variable expression as written, e.g. "kp" or "ctrl.kp".
 * @param id_ Wire ID (0..255), unique in the table.
 * @param variable_ A float lvalue the host may write.
 * @param minimum_ Smallest accepted value.
 * @param maximum_ Largest accepted value.
 * @param unit_ Unit text such as "°C", or "" for none.
 * @par Usage
 * static const KymoParameter params[] = { KYMO_PARAM(100, kp, 0.0f, 10.0f, "") };
 */
#define KYMO_PARAM(id_, variable_, minimum_, maximum_, unit_) \
    { (uint8_t)(id_), &(variable_), (float)(minimum_), (float)(maximum_), #variable_, (unit_) }

/**
 * @brief One parameter the host may write.
 *
 * @details Owned by the application, typically a const table. The receiver keeps
 * a pointer to the table, so it must outlive the receiver.
 * @par Usage
 * { 100u, &kp, 0.0f, 10.0f } lets the host set kp between 0 and 10 via ID 100.
 */
typedef struct {
    /**
     * @brief Wire ID of writes to this parameter and of its readback.
     * @details Input; unique within the table. Choose IDs that the measurement
     * channels do not use, so readbacks are not mixed with measured values.
     */
    uint8_t id;
    /**
     * @brief Application variable that accepted writes update.
     * @details Input; must not be NULL and must stay valid while the receiver is used.
     */
    float *target;
    /**
     * @brief Smallest accepted value, inclusive.
     * @details Input; finite and not greater than maximum.
     */
    float minimum;
    /**
     * @brief Largest accepted value, inclusive.
     * @details Input; finite and not smaller than minimum.
     */
    float maximum;
    /**
     * @brief Name shown by the host, e.g. "kp".
     * @details Input; NULL or up to KYMO_PARAM_NAME_MAX bytes. NULL sends an empty
     * name and the host shows the ID instead. Must outlive the receiver.
     */
    const char *name;
    /**
     * @brief Unit shown by the host, e.g. "°C".
     * @details Input; NULL or up to KYMO_PARAM_UNIT_MAX bytes; NULL means none.
     */
    const char *unit;
} KymoParameter;

/**
 * @brief Receiver state: frame reassembly, pending readbacks and counters.
 *
 * @details Treat all fields as read-only after KymoParam_Init; the counters are
 * diagnostics and wrap modulo 2^32.
 * @par Usage
 * Declare one static KymoParamReceiver per receiving transport.
 */
typedef struct {
    /** @brief Registered parameter table (not owned). */
    const KymoParameter *table;
    /** @brief Number of table entries (1..KYMO_MAX_PARAMETERS). */
    size_t count;
    /** @brief Frame reassembly shared with the other back-channel modules. */
    KymoControlRx rx;
    /** @brief One bit per table index: a readback is due. */
    uint8_t pending[(KYMO_MAX_PARAMETERS + 7u) / 8u];
    /** @brief Table index where the next readback search starts (round robin). */
    size_t cursor;
    /** @brief Table index of the next description to send; count when none is due. */
    size_t describe_next;
    /** @brief Writes applied to a parameter. */
    uint32_t accepted;
    /** @brief Valid frames refused: unknown ID or value out of range. */
    uint32_t rejected;
    /** @brief Frames discarded: unsupported descriptor, NO_CRC, bad CRC or extra fields. */
    uint32_t invalid;
} KymoParamReceiver;

/**
 * @brief Validate a parameter table and reset the receiver.
 *
 * @details Rejects NULL arguments, an empty table, more than KYMO_MAX_PARAMETERS
 * entries, a NULL target, non-finite or inverted limits and duplicate IDs. Current
 * parameter values are not checked or changed. On failure the receiver is unusable
 * until a successful Init.
 * @param[out] receiver Receiver to reset; NULL is rejected.
 * @param[in] table Parameter table that outlives the receiver; NULL is rejected.
 * @param[in] count Number of entries, 1..KYMO_MAX_PARAMETERS.
 * @return KYMO_OK, or KYMO_BAD_CONFIG for an invalid table or argument.
 * @par Usage
 * Call once during start-up, before the first KymoParam_Feed.
 */
KymoStatus KymoParam_Init(KymoParamReceiver *receiver, const KymoParameter *table,
                          size_t count);

/**
 * @brief Consume received bytes and apply every valid parameter write in them.
 *
 * @details Bytes may arrive in any split; a frame may span calls. Bytes outside a
 * frame are skipped until the next sync marker. Constant work per byte plus one
 * table lookup per complete frame.
 * @param[in,out] receiver Initialized receiver; NULL or uninitialized input is ignored.
 * @param[in] bytes Received bytes; NULL is treated as empty.
 * @param[in] length Number of readable bytes.
 * @return Number of writes applied during this call.
 * @par Usage
 * Feed whatever a non-blocking read returned, then drain KymoParam_NextReadback.
 */
size_t KymoParam_Feed(KymoParamReceiver *receiver, const uint8_t *bytes, size_t length);

/**
 * @brief Apply one compact write received over a link with its own framing (e.g. CAN).
 *
 * @details @p data is exactly KYMO_PARAM_COMPACT_SIZE bytes: parameter ID, then the
 * value as little-endian binary32. The link (CAN) provides framing and checksum, so
 * there is no sync, descriptor or CRC. The same rules as KymoParam_Feed apply:
 * registered ID, finite value inside the limits, readback queued for accepted and
 * out-of-range writes. A wrong length or a non-finite value counts as invalid.
 * Parameter discovery is not available this way; the host defines such parameters.
 * @param[in,out] receiver Initialized receiver; NULL or uninitialized input is ignored.
 * @param[in] data Received data field; NULL is ignored.
 * @param[in] length Its length; must be KYMO_PARAM_COMPACT_SIZE.
 * @return 1 when the write was applied, otherwise 0.
 * @par Usage
 * In the CAN receive path, with app_param_can_id the tx_id configured in KymoStudio:
 * if (msg.id == app_param_can_id) KymoParam_FeedCompact(&receiver, msg.data, msg.len);
 */
size_t KymoParam_FeedCompact(KymoParamReceiver *receiver, const uint8_t *data, size_t length);

/**
 * @brief Take the next due readback.
 *
 * @details Fills id and the parameter's current value; flags and the remaining
 * fields are zero. The application may add a timestamp before encoding. Taking a
 * readback clears it, so encode and send it in the same step.
 * @param[in,out] receiver Initialized receiver; NULL is rejected.
 * @param[out] point Destination; NULL is rejected.
 * @return KYMO_OK with a readback, KYMO_IDLE if none is due, KYMO_BAD_CONFIG for NULL input.
 * @par Usage
 * while (KymoParam_NextReadback(&receiver, &point) == KYMO_OK) { encode and send point; }
 */
KymoStatus KymoParam_NextReadback(KymoParamReceiver *receiver, KymoDataPoint *point);

/**
 * @brief Encode the next due parameter description after a list request.
 *
 * @details Descriptions go out in table order, one per call. A new list request
 * restarts the sequence. The frame is complete and CRC-protected; send it as is.
 * Taking a description advances the sequence, so send it in the same step.
 * @param[in,out] receiver Initialized receiver; NULL yields zero.
 * @param[out] output Frame buffer; NULL yields zero.
 * @param[in] capacity Writable bytes; KYMO_PARAM_FRAME_MAX_SIZE fits every description.
 * @return Frame length, or zero when none is due or the buffer is too small
 *         (the description then stays due).
 * @par Usage
 * while ((n = KymoParam_NextDescription(&receiver, frame, sizeof(frame))) != 0u) { send(frame, n); }
 */
size_t KymoParam_NextDescription(KymoParamReceiver *receiver, uint8_t *output, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* KYMO_EFFECTIVE_PARAMETERS */

/** @} */
#endif /* KYMO_PARAM_H */
