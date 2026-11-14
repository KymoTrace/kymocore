/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_param.cpp
 * @brief Param-specific bindings and complete initialization.
 */
#include "kymo_param.h"
#include "../detail/kymo_internal.h"
static KymoCoreEndpointParam sFeature;
#include "../demo_values.h"
static const uint8_t GAIN_PARAMETER_ID = 100u;
static const float GAIN_MINIMUM = 0.0f;
static const float GAIN_MAXIMUM = 10.0f;
static const KymoParameter PARAMETERS[] = {
    {GAIN_PARAMETER_ID, &exampleGain, GAIN_MINIMUM, GAIN_MAXIMUM, "gain", ""}};

/** @brief Fixed param resources, callbacks and feature settings. */
static const KymoCoreParamConfig sParamConfig = {PARAMETERS, KYMO_COUNT_OF(PARAMETERS)};

/****************************************************************************************************
 * exampleParamConfig
 ***************************************************************************************************/
const KymoCoreParamConfig &exampleParamConfig(void)
{
    return sParamConfig;
}

/****************************************************************************************************
 * KymoApp_ParamInit
 ***************************************************************************************************/
KymoStatus KymoApp_ParamInit(const KymoCoreParamConfig *config)
{
    return KymoCoreEndpointParam_init(exampleEndpoint(), &sFeature, config);
}
