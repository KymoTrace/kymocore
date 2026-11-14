/* SPDX-License-Identifier: MIT */
#include <string.h>
#include "kymo_modules.h"

static uint64_t clockRead(void *user)
{
    (void)user;
    return 0u;
}

static uint8_t coreRead(void *user)
{
    (void)user;
    return 0u;
}

static uint32_t contextRead(void *user)
{
    (void)user;
    return 1u;
}

int main(void)
{
    KymoTraceConfig config;
    KymoTraceRecorder recorder;
    KymoTraceEvent event;
    uint8_t payload[36];
    int result = 0;
    memset(&config, 0, sizeof(config));
    memset(&event, 0, sizeof(event));
    config.clock_ticks = clockRead;
    config.core_id = coreRead;
    config.current_context = contextRead;
    if (KymoTrace_init(&recorder, &config) != KYMO_OK)
    {
        result = 1;
    }
    (void)KymoTraceProtocol_encodeBatch(0u, &event, 1u, payload, sizeof(payload));
    if (KymoMux_service((KymoMux *)0) != KYMO_BAD_CONFIG)
    {
        result = 1;
    }
    return result;
}
