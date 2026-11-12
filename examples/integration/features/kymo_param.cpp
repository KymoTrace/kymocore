/* SPDX-License-Identifier: MIT */
/** @file kymo_param.cpp
 * @brief Param-specific bindings and complete initialization.
 */
#include "kymo_param.h"
#include "../detail/kymo_internal.h"
static KymoEndpointParam sFeature;
#include "../demo_values.h"
static const uint8_t GAIN_PARAMETER_ID = 100u;
static const float GAIN_MINIMUM = 0.0f;
static const float GAIN_MAXIMUM = 10.0f;
static const KymoParameter PARAMETERS[] = {
    {GAIN_PARAMETER_ID, &exampleGain, GAIN_MINIMUM, GAIN_MAXIMUM, "gain", ""}};
/*******************************************************************************
 * exampleParamConfig
 ******************************************************************************/
KymoEndpointParamConfig exampleParamConfig(void)
{
    KymoEndpointParamConfig config = {};
    config.table = PARAMETERS;
    config.count = KYMO_COUNT_OF(PARAMETERS);
    return config;
}
/*******************************************************************************
 * KymoApp_ParamInit
 ******************************************************************************/
KymoStatus KymoApp_ParamInit(const KymoEndpointParamConfig *config)
{
    return KymoEndpointParam_Init(exampleEndpoint(), &sFeature, config);
}
