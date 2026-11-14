/* SPDX-License-Identifier: MIT */
/** @file kymo_core_watch.cpp
 * @brief Feature-owned state and application-to-dispatcher configuration.
 */
#include "../../kymo_core_watch.h"
#if KYMO_EFFECTIVE_CORE && KYMO_EFFECTIVE_WATCH
#include "kymo_core_endpoint_watch.h"
#include "../../core/detail/kymo_core_internal.h"
static KymoCoreEndpointWatch defaultWatch = {};

/****************************************************************************************************
 * KymoCoreWatch_init
 ***************************************************************************************************/
/* Map configuration and register only on successful validation. */
KymoStatus KymoCoreWatch_init(const KymoCoreWatchConfig *pConfig)
{
    KymoCoreEndpointWatchConfig internalConfig = {};
    KymoStatus status = KYMO_BAD_CONFIG;
    if (pConfig != nullptr)
    {
        internalConfig.watch.read = pConfig->readValueCbk;
        internalConfig.watch.read_context = pConfig->pReadContext;
        internalConfig.watch.ranges = pConfig->pRangesAv;
        internalConfig.watch.range_count = pConfig->rangeCount;
        internalConfig.watch.build_id = pConfig->pBuildIdAv;
        internalConfig.watch.build_id_length = pConfig->buildIdSizeBytes;
        internalConfig.first_channel = pConfig->firstChannel;
        internalConfig.last_channel = pConfig->lastChannel;
    }
    status = KymoCoreEndpointWatch_init(KymoCore_getEndpoint(), &defaultWatch,
                                        pConfig != nullptr ? &internalConfig : nullptr);
    return status;
}
#endif
