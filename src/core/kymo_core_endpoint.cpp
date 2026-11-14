/* SPDX-License-Identifier: MIT */
/**
 * @file kymo_core_endpoint.cpp
 * @brief Serialized feature coordination using the existing KymoTx sender.
 */
#include "detail/kymo_core_endpoint_internal.h"
#if KYMO_EFFECTIVE_ENDPOINT
#include "../binary/embedded_bits.h"

/****************************************************************************************************
 * KymoCoreEndpoint_fault
 ***************************************************************************************************/
void KymoCoreEndpoint_fault(KymoCoreEndpoint *endpoint, KymoStatus status, uint8_t feature)
{
    if ((endpoint != nullptr) && (endpoint->phase != KYMO_ENDPOINT_UNINITIALIZED) &&
        (endpoint->phase != KYMO_ENDPOINT_FAULT))
    {
        endpoint->error = status;
        endpoint->error_feature = feature;
        endpoint->phase = KYMO_ENDPOINT_FAULT;
    }
}

/****************************************************************************************************
 * KymoCoreEndpoint_prepare
 ***************************************************************************************************/
KymoStatus KymoCoreEndpoint_prepare(KymoCoreEndpoint *endpoint, KymoCoreEndpointFeature *binding,
                                    uint8_t id, size_t capacity, uint8_t needsClock,
                                    uint8_t needsRead)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (endpoint != nullptr)
    {
        if (endpoint->phase == KYMO_ENDPOINT_FAULT)
        {
            status = endpoint->error;
        }
        else if (endpoint->phase == KYMO_ENDPOINT_CONFIGURING)
        {
            if ((binding != nullptr) && (binding->endpoint != nullptr))
            {
                status = KYMO_REJECTED;
            }
            else if (binding != nullptr)
            {
                status = KYMO_OK;
                for (uint8_t index = 0u; index < endpoint->count; ++index)
                {
                    if (endpoint->features[index]->id == id)
                    {
                        status = KYMO_REJECTED;
                    }
                }
                if ((status == KYMO_OK) &&
                    ((endpoint->count >= KYMO_ENDPOINT_FEATURE_CAPACITY) ||
                     (endpoint->config.tx.capacity < capacity) ||
                     ((needsClock != 0u) && (endpoint->config.clock_ms == nullptr)) ||
                     ((needsRead != 0u) && (endpoint->config.read == nullptr))))
                {
                    status = KYMO_BAD_CONFIG;
                }
                if (status == KYMO_OK)
                {
                    binding->id = id;
                }
            }
            if (status != KYMO_OK)
            {
                KymoCoreEndpoint_fault(endpoint, status, id);
            }
        }
        else if (endpoint->phase != KYMO_ENDPOINT_UNINITIALIZED)
        {
            status = KYMO_REJECTED;
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpoint_finish
 ***************************************************************************************************/
KymoStatus KymoCoreEndpoint_finish(KymoCoreEndpoint *endpoint, KymoCoreEndpointFeature *binding,
                                   KymoStatus status)
{
    if ((endpoint != nullptr) && (endpoint->phase == KYMO_ENDPOINT_CONFIGURING))
    {
        if ((status == KYMO_OK) && (binding != nullptr))
        {
            uint8_t position = endpoint->count;

            /* Stable feature order, independent of the application's Init order. */
            while ((position > 0u) && (endpoint->features[position - 1u]->id > binding->id))
            {
                endpoint->features[position] = endpoint->features[position - 1u];
                --position;
            }
            endpoint->features[position] = binding;
            binding->endpoint = endpoint;
            ++endpoint->count;
            endpoint->source_count = (uint8_t)(endpoint->source_count + binding->sources);
        }
        else
        {
            KymoCoreEndpoint_fault(endpoint, status,
                                   binding != nullptr ? binding->id : (uint8_t)KYMO_ENDPOINT_BASE);
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpoint_reserve
 ***************************************************************************************************/
KymoStatus KymoCoreEndpoint_reserve(KymoCoreEndpoint *endpoint, uint8_t channel)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (emb_bitset_test(endpoint->channels, sizeof(endpoint->channels), channel) == 0u)
    {
        (void)emb_bitset_set(endpoint->channels, sizeof(endpoint->channels), channel);
        status = KYMO_OK;
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpoint_copy
 ***************************************************************************************************/
size_t KymoCoreEndpoint_copy(void *user, const uint8_t *bytes, size_t length)
{
    KymoCoreEndpoint *endpoint = static_cast<KymoCoreEndpoint *>(user);
    size_t accepted = 0u;
    KymoStatus status = KymoTx_submit(&endpoint->tx, bytes, length);
    if (status == KYMO_OK)
    {
        accepted = length;
    }
    else
    {
        /* Only the selected source may submit; BUSY here is a broken invariant. */
        KymoCoreEndpoint_fault(endpoint, status == KYMO_BUSY ? KYMO_BAD_CONFIG : status,
                               endpoint->features[endpoint->cursor]->id);
    }
    return accepted;
}

/****************************************************************************************************
 * KymoCoreEndpoint_sample
 ***************************************************************************************************/
KymoStatus KymoCoreEndpoint_sample(KymoCoreEndpoint *endpoint, KymoStatus status,
                                   const KymoDataPoint *point, uint32_t now)
{
    uint8_t frame[KYMO_FRAME_MAX_SIZE] = {};
    (void)now;
    if (status == KYMO_OK)
    {
        KymoDataPoint timed = *point;
#if KYMO_EFFECTIVE_TIMESTAMP
        timed.flags = (uint8_t)(timed.flags | KYMO_FLAG_TIMESTAMP);
        timed.timestamp_ms = now;
#endif
        size_t length = KymoProtocol_encode(&timed, frame, sizeof(frame));
        if (length == 0u)
        {
            status = KYMO_BAD_SAMPLE;
        }
        else if (KymoCoreEndpoint_copy(endpoint, frame, length) != length)
        {
            status = endpoint->error;
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpoint_buffersOverlap
 ***************************************************************************************************/
static uint8_t KymoCoreEndpoint_buffersOverlap(const KymoCoreEndpointConfig *config)
{
    uintptr_t rx = (uintptr_t)config->rx_buffer;
    uintptr_t tx = (uintptr_t)config->tx.buffer;
    uint8_t overlap = 0u;
    if ((config->read != nullptr) && (config->rx_buffer != nullptr) &&
        (config->tx.buffer != nullptr))
    {
        overlap =
            (uint8_t)(rx <= tx ? (tx - rx < config->rx_capacity) : (rx - tx < config->tx.capacity));
    }
    return overlap;
}

/****************************************************************************************************
 * KymoCoreEndpoint_init
 ***************************************************************************************************/
KymoStatus KymoCoreEndpoint_init(KymoCoreEndpoint *endpoint, const KymoCoreEndpointConfig *config)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (endpoint != nullptr)
    {
        if (endpoint->phase != KYMO_ENDPOINT_UNINITIALIZED)
        {
            status = KYMO_REJECTED;
        }
        else if ((config != nullptr) && (KymoCoreEndpoint_buffersOverlap(config) == 0u) &&
                 ((config->read == nullptr) ||
                  ((config->rx_buffer != nullptr) && (config->rx_capacity != 0u))))
        {
            status = KymoTx_init(&endpoint->tx, &config->tx);
            if (status == KYMO_OK)
            {
                endpoint->config = *config;
                endpoint->phase = KYMO_ENDPOINT_CONFIGURING;
            }
        }
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpoint_receiveBytes
 ***************************************************************************************************/
static uint8_t KymoCoreEndpoint_receiveBytes(KymoCoreEndpoint *endpoint)
{
    uint8_t hasReceiver = 0u;
    uint8_t worked = 0u;
    for (uint8_t index = 0u; index < endpoint->count; ++index)
    {
        hasReceiver = (uint8_t)(hasReceiver || (endpoint->features[index]->feed != nullptr));
    }
    if ((hasReceiver != 0u) && (endpoint->phase == KYMO_ENDPOINT_RUNNING))
    {
        size_t length =
            endpoint->config.read(endpoint->config.tx.transport_user, endpoint->config.rx_buffer,
                                  endpoint->config.rx_capacity);
        if (length > endpoint->config.rx_capacity)
        {
            KymoCoreEndpoint_fault(endpoint, KYMO_IO_ERROR, KYMO_ENDPOINT_TRANSPORT);
        }
        else if (length != 0u)
        {
            worked = 1u;
            for (uint8_t index = 0u; index < endpoint->count; ++index)
            {
                KymoCoreEndpointFeature *binding = endpoint->features[index];
                if (binding->feed != nullptr)
                {
                    binding->feed(binding->user, endpoint->config.rx_buffer, length);
                }
            }
        }
    }
    return worked;
}

/****************************************************************************************************
 * KymoCoreEndpoint_produceFrame
 ***************************************************************************************************/
static KymoStatus KymoCoreEndpoint_produceFrame(KymoCoreEndpoint *endpoint)
{
    KymoCoreEndpointFeature *binding = endpoint->features[endpoint->cursor];
    uint8_t draining = (uint8_t)(endpoint->phase == KYMO_ENDPOINT_DRAINING);
    uint32_t now = endpoint->config.clock_ms != nullptr
                       ? endpoint->config.clock_ms(endpoint->config.clock_user)
                       : 0u;
    KymoStatus status = binding->produce(binding->user, endpoint->source, now, draining);
    if (status == KYMO_BAD_SAMPLE)
    {
        endpoint->error_feature = binding->id;
    }
    else if ((status != KYMO_OK) && (status != KYMO_IDLE) && (status != KYMO_BUSY) &&
             (status != KYMO_SKIPPED))
    {
        KymoCoreEndpoint_fault(endpoint, status, binding->id);
    }
    if ((draining != 0u) && (status == KYMO_IDLE) && (endpoint->tx.length == 0u))
    {
        ++endpoint->drain_idle;
        if (endpoint->drain_idle >= endpoint->source_count)
        {
            endpoint->phase = KYMO_ENDPOINT_STOPPED;
        }
    }
    else
    {
        endpoint->drain_idle = 0u;
    }
    ++endpoint->source;
    if (endpoint->source >= binding->sources)
    {
        endpoint->source = 0u;
        endpoint->cursor = (uint8_t)((endpoint->cursor + 1u) % endpoint->count);
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpoint_main
 ***************************************************************************************************/
KymoStatus KymoCoreEndpoint_main(KymoCoreEndpoint *endpoint)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if ((endpoint != nullptr) && (endpoint->phase != KYMO_ENDPOINT_UNINITIALIZED))
    {
        if (endpoint->phase == KYMO_ENDPOINT_CONFIGURING)
        {
            if (endpoint->count == 0u)
            {
                KymoCoreEndpoint_fault(endpoint, KYMO_BAD_CONFIG, KYMO_ENDPOINT_BASE);
            }
            else
            {
                endpoint->phase = KYMO_ENDPOINT_RUNNING;
            }
        }
        status = KYMO_IDLE;
        if ((endpoint->phase == KYMO_ENDPOINT_RUNNING) ||
            (endpoint->phase == KYMO_ENDPOINT_DRAINING))
        {
            if (endpoint->config.service != nullptr)
            {
                endpoint->config.service(endpoint->config.tx.transport_user);
            }
            uint8_t worked = KymoCoreEndpoint_receiveBytes(endpoint);
            if (endpoint->phase != KYMO_ENDPOINT_FAULT)
            {
                size_t queued = endpoint->tx.length;
                status = KymoTx_service(&endpoint->tx);
                worked = (uint8_t)(worked || ((queued != 0u) && (endpoint->tx.length == 0u)));
                if (status == KYMO_IO_ERROR)
                {
                    KymoCoreEndpoint_fault(endpoint, status, KYMO_ENDPOINT_TRANSPORT);
                }
                else if (status == KYMO_IDLE)
                {
                    status = KymoCoreEndpoint_produceFrame(endpoint);
                }
                if ((status == KYMO_IDLE) || (status == KYMO_OK) || (status == KYMO_BUSY) ||
                    (status == KYMO_SKIPPED))
                {
                    if ((endpoint->tx.length != 0u) || (endpoint->phase == KYMO_ENDPOINT_DRAINING))
                    {
                        status = KYMO_BUSY;
                    }
                    else if ((worked != 0u) || (status == KYMO_OK))
                    {
                        status = KYMO_OK;
                    }
                    else
                    {
                        status = KYMO_IDLE;
                    }
                }
            }
        }
        if (endpoint->phase == KYMO_ENDPOINT_FAULT)
        {
            status = endpoint->error;
        }
        else if (endpoint->phase == KYMO_ENDPOINT_STOPPED)
        {
            status = KYMO_IDLE;
        }
        endpoint->last_status = status;
    }
    return status;
}

/****************************************************************************************************
 * KymoCoreEndpoint_requestStop
 ***************************************************************************************************/
KymoStatus KymoCoreEndpoint_requestStop(KymoCoreEndpoint *endpoint)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    if (endpoint != nullptr)
    {
        if (endpoint->phase == KYMO_ENDPOINT_RUNNING)
        {
            endpoint->phase = KYMO_ENDPOINT_DRAINING;
            endpoint->drain_idle = 0u;
            for (uint8_t index = 0u; index < endpoint->count; ++index)
            {
                KymoCoreEndpointFeature *binding = endpoint->features[index];
                if (binding->stop != nullptr)
                {
                    binding->stop(binding->user);
                }
            }
            status = KYMO_OK;
        }
        else if ((endpoint->phase == KYMO_ENDPOINT_DRAINING) ||
                 (endpoint->phase == KYMO_ENDPOINT_STOPPED))
        {
            status = KYMO_OK;
        }
        else if (endpoint->phase == KYMO_ENDPOINT_FAULT)
        {
            status = endpoint->error;
        }
        else if (endpoint->phase == KYMO_ENDPOINT_CONFIGURING)
        {
            status = KYMO_REJECTED;
        }
    }
    return status;
}
#endif
