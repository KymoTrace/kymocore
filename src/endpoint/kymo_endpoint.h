/* SPDX-License-Identifier: MIT */
/** @file kymo_endpoint.h
 * @brief Shared Init/Main coordinator; individual features have separate headers.
 * @details Single serialized owner, no heap, SDK or implicit tasks. Config and
 * callbacks must remain valid. Feature contexts and buffers must not move. Stop
 * all concurrent producers before RequestStop; Init/Main are not ISR-safe.
 * @par Usage Init the base, initialize selected features, then call Main cyclically.
 */
#ifndef KYMO_ENDPOINT_H
#define KYMO_ENDPOINT_H
#include "detail/kymo_feature.h"
#include "../transport/kymo_tx.h"
#if KYMO_EFFECTIVE_ENDPOINT || defined(DOXYGEN)
#ifdef __cplusplus
extern "C"
{
#endif
/** @brief Maximum registered feature count for this build.
 * @par Usage Internal array bound; unused features allocate no feature context. */
#define KYMO_ENDPOINT_FEATURE_COUNT                                              \
    (KYMO_EFFECTIVE_RUNTIME + KYMO_EFFECTIVE_PARAMETERS + KYMO_EFFECTIVE_WATCH + \
     KYMO_EFFECTIVE_MEMORY + KYMO_EFFECTIVE_TRACE)
/** @brief Nonzero C-compatible array bound, including an empty build.
 * @par Usage Internal allocation only. */
#define KYMO_ENDPOINT_FEATURE_CAPACITY \
    ((KYMO_ENDPOINT_FEATURE_COUNT > 0) ? KYMO_ENDPOINT_FEATURE_COUNT : 1)
/** @brief Number of DATA channel IDs on the wire. @par Usage Reservation bound. */
#define KYMO_ENDPOINT_CHANNEL_COUNT 256u
    /** @brief Lifecycle phases. @par Usage Inspect phase read-only for diagnostics. */
    typedef enum
    {
        KYMO_ENDPOINT_UNINITIALIZED, /**< Zero-initialized, not ready. */
        KYMO_ENDPOINT_CONFIGURING,   /**< Feature Init is allowed. */
        KYMO_ENDPOINT_RUNNING,       /**< Main owns the frozen configuration. */
        KYMO_ENDPOINT_DRAINING,      /**< No RX/new samples; finish existing output. */
        KYMO_ENDPOINT_STOPPED,       /**< All sources and physical TX have drained. */
        KYMO_ENDPOINT_FAULT          /**< Terminal configuration/transport failure. */
    } KymoEndpointPhase;
    /** @brief Diagnostic source, independent of enabled modules.
     * @par Usage Compare error_feature with these identifiers. */
    typedef enum
    {
        KYMO_ENDPOINT_BASE,     /**< Base configuration. */
        KYMO_ENDPOINT_PLOT,     /**< Plot samples. */
        KYMO_ENDPOINT_PARAM,    /**< Parameter reception/replies. */
        KYMO_ENDPOINT_WATCH,    /**< Watch reception/sampling. */
        KYMO_ENDPOINT_MEMORY,   /**< Memory information/sampling. */
        KYMO_ENDPOINT_TRACE,    /**< Trace recording/output. */
        KYMO_ENDPOINT_TRANSPORT /**< Physical port contract. */
    } KymoEndpointFeatureId;
    /** @brief Nonblocking read into caller storage.
     * @param[in,out] user Transport context.
     * @param[out] bytes Writable span.
     * @param[in] capacity Maximum bytes permitted.
     * @return Bytes copied, 0..capacity. Larger values fault the Endpoint.
     * @par Usage Read at most one bounded port fragment. */
    typedef size_t (*KymoEndpointReadFn)(void *user, uint8_t *bytes, size_t capacity);
    /** @brief Base resources only; feature configuration belongs in feature headers.
     * @details Copied on Init; referenced storage/contexts outlive the Endpoint.
     * @par Usage Zero-initialize then assign fields by name. */
    typedef struct
    {
        KymoTxConfig tx;         /**< TX buffer, capacity, write, busy and physical context. */
        KymoEndpointReadFn read; /**< Required by parameter/watch; NULL otherwise allowed. */
        KymoServiceFn service;   /**< Optional bounded port maintenance, once per Main. */
        uint8_t *rx_buffer;      /**< Persistent writable receive buffer, separate from TX. */
        size_t rx_capacity;      /**< RX byte budget per Main; required with read. */
        KymoClockFn clock_ms;    /**< Required by scheduled Plot/Watch/Memory features. */
        void *clock_user;        /**< Clock context; read/service use tx.transport_user. */
    } KymoEndpointConfig;
    /** @brief Application-owned base storage, zero-initialize before first Init.
     * @details Read-only fields after Init. No feature objects are embedded here.
     * There is no reset/reconnect: quiesce the driver and producers before starting
     * a new object lifetime. Driver-borrowed storage must never be invalidated.
     * @par Usage Declare static KymoEndpoint endpoint. */
    struct KymoEndpoint
    {
        KymoEndpointConfig config; /**< Copied base resources. */
        KymoTx tx;                 /**< Sole physical sender. */
        KymoEndpointFeature *features[KYMO_ENDPOINT_FEATURE_CAPACITY]; /**< Registered bindings. */
        uint8_t channels[KYMO_ENDPOINT_CHANNEL_COUNT / 8u];            /**< Reserved DATA IDs. */
        KymoEndpointPhase phase; /**< Current lifecycle phase. */
        KymoStatus error;        /**< First terminal error, or KYMO_OK. */
        KymoStatus last_status;  /**< Most recent Main result, including sample errors. */
        uint8_t error_feature;   /**< Source of terminal error or latest sample error. */
        uint8_t count;           /**< Registered feature count. */
        uint8_t cursor;          /**< Next feature in source rotation. */
        uint8_t source;          /**< Next subsource within that feature. */
        uint8_t source_count;    /**< Total source turns in a rotation. */
        uint8_t drain_idle;      /**< Consecutive empty drain turns. */
    };
    /** @brief Initialize common resources once, without driver callbacks.
     * @param[in,out] endpoint Zero-initialized context.
     * @param[in] config Base resources; NULL is invalid.
     * @return OK, BAD_CONFIG, or REJECTED for repeated Init. Invalid initial config
     * leaves a zero context retryable; registered feature errors are latched.
     * @par Usage Before all feature Init calls. */
    KymoStatus KymoEndpoint_Init(KymoEndpoint *endpoint, const KymoEndpointConfig *config);
    /** @brief Perform bounded RX, one TX write attempt and at most one source turn.
     * @details First Main freezes configuration. No active waits. Busy TX still permits
     * bounded RX. Invalid samples are reported without faulting other sources.
     * @param[in,out] endpoint Initialized base with at least one registered feature.
     * @return Error first, otherwise BUSY while TX/drain remains, OK for work, IDLE
     * for no work or completed stop. NULL/uninitialized yields BAD_CONFIG.
     * @par Usage Call from one superloop/task; handle errors separately from BUSY. */
    KymoStatus KymoEndpoint_Main(KymoEndpoint *endpoint);
    /** @brief Request a finite drain, using subsequent Main calls.
     * @details Quiesce external Trace producers first. No new RX/samples follow.
     * Does not reset or release borrowed bytes; repeated stop requests are harmless.
     * @param[in,out] endpoint Running Endpoint.
     * @return OK when requested/already stopping, REJECTED before first Main,
     * BAD_CONFIG for NULL/uninitialized, or the latched error.
     * @par Usage Optional at shutdown; continue Main until IDLE. */
    KymoStatus KymoEndpoint_RequestStop(KymoEndpoint *endpoint);
#ifdef __cplusplus
}
#endif
#endif
#endif
