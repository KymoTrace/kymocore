/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_param.cpp
 * @brief Default-Core parameter initialization with feature-owned static state.
 */
#include "kymo_core_endpoint_param.h"

#if KYMO_EFFECTIVE_CORE && KYMO_EFFECTIVE_PARAMETERS
#include "../../core/detail/kymo_core_internal.h"
static KymoCoreEndpointParam defaultParameters = {};

/****************************************************************************************************
 * KymoCoreParam_init
 ***************************************************************************************************/
/* Initialize and register the feature-owned state through the existing binding. */
KymoStatus KymoCoreParam_init(const KymoCoreParamConfig *pConfig)
{
    KymoStatus status =
        KymoCoreEndpointParam_init(KymoCore_getEndpoint(), &defaultParameters, pConfig);
    return status;
}
#endif
