/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_app.cpp
 * @brief Select the features used by this demonstration; no RX/TX coordination.
 */
#include "kymo_app.h"
#include "kymo_init.h"
#include "features/kymo_plot.h"
#include "features/kymo_param.h"
#include "features/kymo_watch.h"
#include "features/kymo_memory.h"
#include "features/kymo_trace.h"

/****************************************************************************************************
 * exampleAppInit
 ***************************************************************************************************/
KymoStatus exampleAppInit(void)
{
    KymoStatus status = KymoApp_Init(&exampleCoreConfig());
    if (status == KYMO_OK)
    {
        status = KymoApp_PlotInit(&examplePlotConfig());
    }
    if (status == KYMO_OK)
    {
        status = KymoApp_ParamInit(&exampleParamConfig());
    }
    if (status == KYMO_OK)
    {
        status = KymoApp_WatchInit(&exampleWatchConfig());
    }
    if (status == KYMO_OK)
    {
        status = KymoApp_MemoryInit(&exampleMemoryConfig());
    }
    if (status == KYMO_OK)
    {
        status = KymoApp_TraceInit(&exampleTraceConfig());
    }
    return status;
}
