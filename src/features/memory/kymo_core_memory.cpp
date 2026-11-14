/* SPDX-License-Identifier: MIT */
/** @file kymo_core_memory.cpp
 * @brief Feature-owned state and application-to-dispatcher configuration.
 */
#include "../../kymo_core_memory.h"
#if KYMO_EFFECTIVE_CORE && KYMO_EFFECTIVE_MEMORY
#include "kymo_core_endpoint_memory.h"
#include "../../core/detail/kymo_core_internal.h"
static KymoCoreEndpointMemory defaultMemory = {};

/****************************************************************************************************
 * KymoCoreMemory_init
 ***************************************************************************************************/
/* Map configuration and register only on successful validation. */
KymoStatus KymoCoreMemory_init(const KymoCoreMemoryConfig *pConfig)
{
    KymoStatus status =
        KymoCoreEndpointMemory_init(KymoCore_getEndpoint(), &defaultMemory, pConfig);
    return status;
}

/****************************************************************************************************
 * KymoCoreMemory_requestInfo
 ***************************************************************************************************/
/* Schedule metadata without performing transport I/O. */
void KymoCoreMemory_requestInfo(void)
{
    KymoMemory_requestInfo(&defaultMemory.memory);
}
#endif
