/* SPDX-License-Identifier: MIT */
/** @file kymo_app.cpp
 * @brief Select the features used by this demonstration; no RX/TX coordination.
 */
#include "kymo_app.h"
#include "kymo_init.h"
#include "features/kymo_plot.h"
#include "features/kymo_param.h"
#include "features/kymo_watch.h"
#include "features/kymo_memory.h"
#include "features/kymo_trace.h"
/*******************************************************************************
 * exampleAppInit
 ******************************************************************************/
KymoStatus exampleAppInit(void)
{
    KymoEndpointConfig core = exampleCoreConfig();
    KymoEndpointPlotConfig plot = examplePlotConfig();
    KymoEndpointParamConfig param = exampleParamConfig();
    KymoEndpointWatchConfig watch = exampleWatchConfig();
    KymoEndpointMemoryConfig memory = exampleMemoryConfig();
    KymoEndpointTraceConfig trace = exampleTraceConfig();
    KymoStatus status = KymoApp_Init(&core);
    if (status == KYMO_OK)
    {
        status = KymoApp_PlotInit(&plot);
    }
    if (status == KYMO_OK)
    {
        status = KymoApp_ParamInit(&param);
    }
    if (status == KYMO_OK)
    {
        status = KymoApp_WatchInit(&watch);
    }
    if (status == KYMO_OK)
    {
        status = KymoApp_MemoryInit(&memory);
    }
    if (status == KYMO_OK)
    {
        status = KymoApp_TraceInit(&trace);
    }
    return status;
}
