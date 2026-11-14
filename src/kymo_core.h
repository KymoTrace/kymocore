/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core.h
 * @brief Public Init/Main interface for the library-owned default connection.
 * @details Enable KYMO_ENABLE_CORE. Include selected features separately and
 * initialize them before Main. Serialized calls, no heap or implicit task.
 * Explicit-instance module interfaces remain available via kymo_modules.h.
 * @par Usage KymoCore_init(NULL); then KymoCore_main() for an empty core.
 */
#ifndef KYMO_CORE_H
#define KYMO_CORE_H
#include "kymo_types.h"
#if KYMO_EFFECTIVE_CORE || defined(DOXYGEN)
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Shared transport resources, without feature state.
     * @details Copied on Init. Buffers and callback contexts must outlive all
     * processing. RX and TX must not overlap. Init performs no I/O.
     * @par Usage Zero-initialize and assign named fields before Core Init.
     */
    typedef struct
    {
        /* Callbacks: all callable fields precede resource parameters. */
        /** @brief Required for receiving features. */
        KymoReadBytesFn readBytesCbk;

        /** @brief Required with transport config. */
        KymoWriteBytesFn writeBytesCbk;

        /** @brief Required when write borrows TX storage. */
        KymoIsBusyFn isBusyCbk;

        /** @brief Optional bounded port maintenance. */
        KymoServiceFn serviceCbk;

        /** @brief Optional monotonic millisecond clock. */
        KymoClockMsFn clockMsCbk;

        /* Parameters: contexts, then each array pointer beside its byte size. */
        /** @brief Context for read/write/busy/service callbacks. */
        void *pTransportContext;

        /** @brief Context passed to clockMsCbk. */
        void *pClockContext;

        /** @brief Persistent TX array; required with a config. */
        uint8_t *pTxBufferAv;

        /** @brief TX array capacity; must fit selected frames. */
        size_t txBufferSizeBytes;

        /** @brief Persistent RX array; required with readBytesCbk. */
        uint8_t *pRxBufferAv;

        /** @brief RX array capacity and byte budget per Main. */
        size_t rxBufferSizeBytes;
    } KymoCoreConfig;

    /**
     * @brief Initialize the default core once, without activating any feature.
     * @details NULL starts a transport-free empty core. Features needing transport
     * then cannot initialize. Invalid transport config can be corrected and retried.
     * Successful Init cannot be repeated; no reset or reconnect is provided.
     * @param[in] pConfig Resource configuration, or NULL for an empty core.
     * @return KYMO_OK, KYMO_BAD_CONFIG or KYMO_REJECTED for repeated Init.
     * @par Usage Core Init, selected feature Inits, then Main. Not ISR-safe.
     */
    KymoStatus KymoCore_init(const KymoCoreConfig *pConfig);

    /**
     * @brief Run one bounded turn for initialized features.
     * @details First Main freezes registration, even for an empty core. An empty
     * core returns IDLE without transport calls. Configuration faults stay latched.
     * @return BAD_CONFIG before Init; otherwise processing/error status.
     * @par Usage Call from one main loop/task. BUSY and IDLE are normal. Not ISR-safe.
     */
    KymoStatus KymoCore_main(void);
#ifdef __cplusplus
}
#endif
#endif
#endif
