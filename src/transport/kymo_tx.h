/* SPDX-License-Identifier: MIT */
/** @file kymo_tx.h
 * @brief Optional single-frame sender for a shared byte transport.
 * @details Copies complete frames into caller-owned storage, resumes short writes
 * and retains bytes until the driver releases them. No encoding, RX, scheduling,
 * allocation or Endpoint is involved. Enable KYMO_ENABLE_TX=1 in the whole build.
 * All calls and callbacks for one context must be serialized and non-reentrant.
 * @par Usage Zero-initialize a KymoTx, initialize it once, submit a frame and
 * call KymoTx_Service once per application cycle until KYMO_IDLE.
 */
#ifndef KYMO_TX_H
#define KYMO_TX_H
#include "../kymo_types.h"

#if KYMO_EFFECTIVE_TX || defined(DOXYGEN)
#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Accept a prefix of a frame, or zero for backpressure.
     * @details Copy before returning, or retain the accepted bytes only while the
     * matching busy callback reports nonzero. Message transports accept all or zero.
     * @param[in,out] user Transport context; may be NULL.
     * @param[in] bytes Read-only persistent storage; non-NULL.
     * @param[in] length Offered bytes, greater than zero.
     * @return Accepted bytes, 0..length; a larger value permanently faults the sender.
     * @par Usage Adapt a UART, USB or network driver without platform types.
     */
    typedef size_t (*KymoTxWriteFn)(void *user, const uint8_t *bytes, size_t length);

    /** @brief Application-owned resources and transport callbacks.
     * @details Init copies this structure; buffer and transport context must remain
     * valid for the sender's lifetime. Each sender needs exclusive storage and access
     * to its physical stream; other writers must not interleave bytes.
     * @par Usage Set fields by name, then pass to KymoTx_Init.
     */
    typedef struct
    {
        /** @brief Writable frame storage, separate from the KymoTx object.
         * @par Usage Provide a persistent array; do not modify it after Init. */
        uint8_t *buffer;
        /** @brief Buffer size in bytes, greater than zero.
         * @par Usage Use sizeof(buffer); size for the largest submitted frame. */
        size_t capacity;
        /** @brief Required physical write callback.
         * @par Usage Report exactly the bytes accepted by the driver. */
        KymoTxWriteFn write;
        /** @brief Optional for copying drivers; required for borrowed buffers.
         * @par Usage Report nonzero while unavailable or retaining any bytes. */
        KymoBusyFn busy;
        /** @brief Opaque context passed to both callbacks.
         * @par Usage Store driver state here; NULL is permitted. */
        void *transport_user;
    } KymoTxConfig;

    /** @brief Sender context, owned by the application; no global sender state.
     * @details Zero-initialize before the first Init. Fields are implementation
     * state: do not modify, move or copy an initialized sender or its buffer. There
     * is no reset that could invalidate a borrowed pointer. To start a new lifetime,
     * first stop/quiesce the driver and ensure it retains no bytes, then zero the
     * context and initialize again. Separate contexts may use independent drivers.
     * @par Usage Declare a static KymoTx or use KymoTx tx = {0} in C.
     */
    typedef struct
    {
        /** @brief Copied resource configuration. @par Usage Treat as read-only. */
        KymoTxConfig config;
        /** @brief Queued frame size; zero when free. @par Usage Treat as read-only. */
        size_t length;
        /** @brief Bytes accepted so far. @par Usage Treat as read-only. */
        size_t offset;
        /** @brief Successful initialization flag. @par Usage Initially zero. */
        uint8_t initialized;
        /** @brief Latched driver violation. @par Usage Treat as read-only. */
        uint8_t fault;
    } KymoTx;

    /** @brief Initialize a zero-initialized context once, without calling the driver.
     * @details Invalid configuration leaves the context unchanged and may be retried.
     * Reinitialization is refused even when idle or faulted, protecting borrowed bytes.
     * @param[in,out] tx Zero-initialized sender; NULL is invalid.
     * @param[in] config Resources to copy; NULL is invalid.
     * @return KYMO_OK, KYMO_BAD_CONFIG for missing resources or zero capacity, or
     * KYMO_REJECTED if the context was already initialized.
     * @par Usage Check success before submitting frames.
     */
    KymoStatus KymoTx_Init(KymoTx *tx, const KymoTxConfig *config);

    /** @brief Copy one complete frame into free persistent storage.
     * @details Consumes all bytes on success and none otherwise. The caller can reuse
     * its input immediately after KYMO_OK when separate from the configured buffer.
     * Overlap is supported, but the configured buffer always remains sender-owned.
     * @param[in,out] tx Initialized sender; NULL is invalid.
     * @param[in] bytes Readable frame; NULL is invalid.
     * @param[in] length Frame size, 1..configured capacity.
     * @return KYMO_OK if copied, KYMO_BUSY if occupied, KYMO_BAD_CONFIG for invalid
     * input/uninitialized context, or latched KYMO_IO_ERROR after a driver violation.
     * @par Usage Retry the same frame on BUSY; do not discard it.
     */
    KymoStatus KymoTx_Submit(KymoTx *tx, const uint8_t *bytes, size_t length);

    /** @brief Advance by at most one physical write attempt; never wait in a loop.
     * @details Retains storage until all bytes are accepted AND busy reports zero.
     * A driver accepting more than offered permanently stops callbacks and writes.
     * No timeout/reset can discard pending or borrowed bytes. Driver maintenance
     * remains the application's responsibility; callbacks must themselves be bounded.
     * @param[in,out] tx Initialized sender; NULL is invalid.
     * @return KYMO_IDLE when storage is free (not a link-health indication), KYMO_BUSY
     * while queued/borrowed, KYMO_IO_ERROR after a driver violation, or KYMO_BAD_CONFIG
     * for NULL/uninitialized context.
     * @par Usage Produce another frame after IDLE; service the port each cycle too.
     */
    KymoStatus KymoTx_Service(KymoTx *tx);

#ifdef __cplusplus
}
#endif
#endif /* KYMO_EFFECTIVE_TX */
#endif /* KYMO_TX_H */
