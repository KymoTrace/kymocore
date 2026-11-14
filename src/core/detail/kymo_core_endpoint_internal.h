/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_endpoint_internal.h
 * @brief Private helpers for the built-in feature adapters.
 * @details Not an application extension/registration API.
 * @par Usage Included by Endpoint implementation files only.
 */
#ifndef KYMO_CORE_ENDPOINT_INTERNAL_H
#define KYMO_CORE_ENDPOINT_INTERNAL_H
#include "../kymo_core_endpoint.h"
#include "../../communication/kymo_protocol.h"
#if KYMO_EFFECTIVE_ENDPOINT

/**
 * @brief Check built-in feature ownership and base resources.
 * @param[in,out] endpoint Base; NULL/uninitialized is rejected.
 * @param[in,out] binding Unbound feature; NULL is rejected.
 * @param[in] id Built-in feature identifier.
 * @param[in] capacity Minimum shared frame capacity.
 * @param[in] needsClock Nonzero requires the millisecond clock.
 * @param[in] needsRead Nonzero requires RX.
 * @return OK, BAD_CONFIG, REJECTED or a previously latched error.
 * @par Usage Private first step of a feature Init; no I/O.
 */
KymoStatus KymoCoreEndpoint_prepare(KymoCoreEndpoint *endpoint, KymoCoreEndpointFeature *binding,
                                    uint8_t id, size_t capacity, uint8_t needsClock,
                                    uint8_t needsRead);

/**
 * @brief Attach a successfully initialized binding or latch its failure.
 * @param[in,out] endpoint Base checked by Prepare; NULL is tolerated.
 * @param[in,out] binding Initialized binding; NULL only on failure.
 * @param[in] status Result of the feature setup.
 * @return Unchanged status.
 * @par Usage Final step after Prepare; configuring base only.
 */
KymoStatus KymoCoreEndpoint_finish(KymoCoreEndpoint *endpoint, KymoCoreEndpointFeature *binding,
                                   KymoStatus status);

/**
 * @brief Reserve one previously unused DATA channel.
 * @param[in,out] endpoint Non-NULL configuring base checked by Prepare.
 * @param[in] channel Wire ID, 0..255.
 * @return OK or BAD_CONFIG on collision.
 * @par Usage After successful Prepare; setup failure terminates configuration.
 */
KymoStatus KymoCoreEndpoint_reserve(KymoCoreEndpoint *endpoint, uint8_t channel);

/**
 * @brief Latch the first terminal error on an initialized base.
 * @param[in,out] endpoint Base; NULL/uninitialized is ignored.
 * @param[in] status Terminal error code.
 * @param[in] feature Diagnostic source identifier.
 * @par Usage Private error path; does not release borrowed storage.
 */
void KymoCoreEndpoint_fault(KymoCoreEndpoint *endpoint, KymoStatus status, uint8_t feature);

/**
 * @brief Copy one complete frame into the sole physical sender.
 * @param[in,out] user Non-NULL Endpoint currently processing a source turn.
 * @param[in] bytes Complete encoded frame.
 * @param[in] length Frame size.
 * @return Length on success, zero on a latched failure.
 * @par Usage Private module write callback; TX must be free.
 */
size_t KymoCoreEndpoint_copy(void *user, const uint8_t *bytes, size_t length);

/**
 * @brief Encode and submit a successfully acquired DATA point.
 * @param[in,out] endpoint Non-NULL base with free TX during a source turn.
 * @param[in] status Acquisition result; only OK triggers encoding.
 * @param[in] point Acquired point, required on OK.
 * @param[in] now Endpoint cycle clock in milliseconds; included when timestamps are enabled.
 * @return Acquisition status, OK, BAD_SAMPLE or a terminal TX error.
 * @par Usage Shared private Param/Watch/Memory adapter.
 */
KymoStatus KymoCoreEndpoint_sample(KymoCoreEndpoint *endpoint, KymoStatus status,
                                   const KymoDataPoint *point, uint32_t now);
#endif
#endif
