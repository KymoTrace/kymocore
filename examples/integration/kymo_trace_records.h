/* SPDX-License-Identifier: MIT */
/** @file kymo_trace_records.h
 * @brief Canonical trace descriptions for this fixed native example.
 * @details Static session 1, domain 0, 1000 Hz, context 1, marker 1.
 * Update metadata and dictionary_hash together when changing bindings.
 * @par Usage Queue with KymoMux_QueueAdmin; omit the C string terminator.
 */
#ifndef EXAMPLE_KYMO_TRACE_RECORDS_H
#define EXAMPLE_KYMO_TRACE_RECORDS_H
#include <stdint.h>
/** @brief Canonical JSON body, without the CRC32 added by the mux. */
static const uint8_t exampleTraceMetadata[] =
    "{\"build_id\":\"native-demo-v1\",\"clock\":{\"clock_domain_id\":0,\"counter_b"
    "its\":64,\"frequency_hz\":\"1000\",\"reset_rule\":\"new_session\",\"smp_synchr"
    "onized\":false},\"core_count\":1,\"device_id\":\"integration-example\",\"dic"
    "tionary_hash\":\"484d11bdcb92f60425c3aa23bb201e10ae397b3327a17ea38fb7a"
    "80d47ae560b\",\"event_schema_version\":1,\"features\":{\"flow\":false,\"isr\""
    ":false,\"scheduler\":false,\"trace\":true},\"port_capabilities\":{\"core_st"
    "ate_complete\":false,\"isr_complete\":false,\"kind\":\"generic\",\"scheduler"
    "_complete\":false},\"schema_version\":1,\"wire_version\":2}";
/** @brief Canonical JSON body, without the CRC32 added by the mux. */
static const uint8_t exampleTraceDictionary[] =
    "{\"contexts\":[{\"id\":1,\"kind\":\"main\",\"name\":\"superloop\"}],\"generation\""
    ":1,\"schema_version\":1,\"symbols\":[{\"id\":1,\"kind\":\"marker\",\"name\":\"mai"
    "n cycle\"}]}";
#endif
