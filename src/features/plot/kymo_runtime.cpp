/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
#include "../../kymo_config.h"
#if KYMO_ENABLED
#include "kymo_runtime.h"
#if KYMO_ENABLE_RUNTIME
#include <string.h>

/****************************************************************************************************
 * KymoRuntime_configValid
 ***************************************************************************************************/
static uint8_t KymoRuntime_configValid(const KymoConfig *config)
{
    uint8_t valid = 0;
    if (config != nullptr)
    {
        valid =
            (uint8_t)((config->channels != nullptr) && (config->clock_ms != nullptr) &&
                      (config->sample != nullptr) && (config->write != nullptr) &&
                      (config->channel_count > 0u) && (config->channel_count <= KYMO_MAX_CHANNELS));
        if (valid != 0u)
        {
            uint16_t i;
            uint8_t seen[32] = {};
            for (i = 0; (i < config->channel_count) && (valid != 0u); ++i)
            {
                const KymoChannel *ch = &config->channels[i];
                valid = (uint8_t)((ch->periodMs <= INT32_MAX) &&
                                  emb_u8_only_bits(ch->flags, KYMO_SUPPORTED_FLAGS) &&
                                  !emb_bitset_test(seen, sizeof(seen), ch->id));
                if (valid != 0u)
                {
                    (void)emb_bitset_set(seen, sizeof(seen), ch->id);
                }
            }
        }
    }
    return valid;
}

/****************************************************************************************************
 * KymoRuntime_init
 ***************************************************************************************************/
KymoStatus KymoRuntime_init(KymoContext *context, const KymoConfig *config)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (context != nullptr)
    {
        memset(context, 0, sizeof(*context));
        if (KymoRuntime_configValid(config))
        {
            uint16_t i;
            uint32_t now = config->clock_ms(config->clock_user);
            context->config = *config;
            for (i = 0; i < config->channel_count; ++i)
            {
                context->last_sample_ms[i] = now - config->channels[i].periodMs;
            }
#if KYMO_ENABLE_TIMESTAMP
            context->start_ms = now;
#endif
            context->initialized = 1u;
            status = KYMO_OK;
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoRuntime_transmit
 ***************************************************************************************************/
static KymoStatus KymoRuntime_transmit(KymoContext *context)
{
    KymoStatus status = KYMO_IO_ERROR;
    const KymoConfig *config = &context->config;
    uint8_t remaining = (uint8_t)(context->tx_length - context->tx_offset);
    uint8_t count =
        config->write(config->transport_user, context->tx + context->tx_offset, remaining);
    if (count > remaining)
    {
        context->fault = 1;
    }
    else
    {
        context->tx_offset = (uint8_t)(context->tx_offset + count);
        status = context->tx_offset == context->tx_length ? KYMO_OK : KYMO_BUSY;
    }
    return status;
}

/****************************************************************************************************
 * KymoRuntime_sampleChannel
 ***************************************************************************************************/
static KymoStatus KymoRuntime_sampleChannel(KymoContext *context, uint16_t index, uint32_t now)
{
    KymoStatus status = KYMO_SKIPPED;
    const KymoConfig *config = &context->config;
    const KymoChannel *ch = &config->channels[index];
    KymoSample sample = {};
    context->last_sample_ms[index] = now;
    if (config->sample(config->sample_user, ch->id, &sample))
    {
        KymoDataPoint point = {};
        point.id = ch->id;
        point.flags = ch->flags;
#if KYMO_ENABLE_X
        point.x = sample.x;
#endif
        point.value = sample.value;
#if KYMO_ENABLE_Z
        point.z = sample.z;
#endif
#if KYMO_ENABLE_TIMESTAMP
        point.timestamp_ms = now - context->start_ms;
#endif
        context->tx_offset = 0;
        context->tx_length = (uint8_t)KymoProtocol_encode(&point, context->tx, sizeof(context->tx));
        if (context->tx_length == 0u)
        {
            status = KYMO_BAD_SAMPLE;
        }
        else
        {
            status = KymoRuntime_transmit(context);
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoRuntime_sampleNextDue
 ***************************************************************************************************/
static KymoStatus KymoRuntime_sampleNextDue(KymoContext *context)
{
    KymoStatus status = KYMO_IDLE;
    const KymoConfig *config = &context->config;
    uint32_t now = config->clock_ms(config->clock_user);
    uint16_t checked;
    uint8_t selected = 0;
    for (checked = 0; (checked < config->channel_count) && !selected; ++checked)
    {
        uint16_t index = context->next_channel;
        const KymoChannel *ch = &config->channels[index];
        context->next_channel = (uint16_t)(index + 1u);
        if (context->next_channel == config->channel_count)
        {
            context->next_channel = 0;
        }
        if ((uint32_t)(now - context->last_sample_ms[index]) >= ch->periodMs)
        {
            selected = 1;
            status = KymoRuntime_sampleChannel(context, index, now);
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoRuntime_main
 ***************************************************************************************************/
KymoStatus KymoRuntime_main(KymoContext *context)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if ((context != nullptr) && (context->initialized != 0u))
    {
        const KymoConfig *config = &context->config;
        if (config->service != nullptr)
        {
            config->service(config->transport_user);
        }
        if (context->fault)
        {
            status = KYMO_IO_ERROR;
        }
        else if ((config->busy != nullptr) && config->busy(config->transport_user))
        {
            status = KYMO_BUSY;
        }
        else if (context->tx_offset < context->tx_length)
        {
            status = KymoRuntime_transmit(context);
        }
        else
        {
            status = KymoRuntime_sampleNextDue(context);
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoRuntime_sampleBound
 ***************************************************************************************************/
uint8_t KymoRuntime_sampleBound(void *user, uint8_t id, KymoSample *sample)
{
    uint8_t found = 0u;
    const KymoPlotBinding *binding = (const KymoPlotBinding *)user;
    if ((binding != nullptr) && (sample != nullptr) && (binding->pChannelsAv != nullptr) &&
        (binding->pValuesAv != nullptr))
    {
        uint16_t i;
        for (i = 0u; (i < binding->channelCount) && (found == 0u); ++i)
        {
            if ((binding->pChannelsAv[i].id == id) && (binding->pValuesAv[i] != nullptr) &&
                emb_u8_only_bits(binding->pChannelsAv[i].flags, KYMO_FLAG_TIMESTAMP))
            {
                sample->value = *binding->pValuesAv[i];
                found = 1u;
            }
        }
    }
    return found;
}

#endif /* KYMO_ENABLE_RUNTIME */

#endif /* KYMO_ENABLED */
