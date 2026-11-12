/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/**
 * @file main.cpp
 * @brief ESP-IDF example: stream a sine and a sawtooth to KymoStudio over UART0.
 * @details Channel 0 is a 0.5 Hz sine, channel 1 a 1 Hz sawtooth, each sampled every
 * 20 ms. UART0 is the USB serial port of most boards and also the IDF console, so
 * logging is switched off: text would corrupt the binary stream. The write callback
 * only fills the free UART FIFO and never waits, as KymoCore expects.
 */
#include <math.h>
#include <stdint.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <KymoCore.h>

static const uart_port_t kPort = UART_NUM_0;
static const float kTwoPi = 6.28318531f;
static KymoContext sContext = {};
static const KymoChannel sChannels[] = {{20U, 0U, 0U}, {20U, 1U, 0U}};

/** @brief Read the monotonic millisecond clock.
 * @param[in] pUser Unused; pass nullptr.
 * @return Milliseconds since boot, wrapping modulo 2^32.
 */
/*******************************************************************************
 * readClockMs
 ******************************************************************************/
static uint32_t readClockMs(void *pUser)
{
    uint32_t result = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    (void)pUser;
    return result;
}

/** @brief Supply the test signal of a channel; replace with your measurements.
 * @param[in] pUser Unused; pass nullptr.
 * @param[in] channelId 0 for the sine, 1 for the sawtooth.
 * @param[out] pSample Destination; null is rejected.
 * @return One for a valid sample, otherwise zero.
 */
/*******************************************************************************
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *pUser, uint8_t channelId, KymoSample *pSample)
{
    uint8_t result = 0U;
    float seconds = static_cast<float>(esp_timer_get_time()) / 1000000.0f;
    (void)pUser;
    if ((pSample != nullptr) && (channelId <= 1U))
    {
        if (channelId == 0U)
        {
            pSample->value = sinf(kTwoPi * 0.5f * seconds);
        }
        else
        {
            pSample->value = fmodf(seconds, 1.0f);
        }
        result = 1U;
    }
    return result;
}

/** @brief Hand frame bytes to the free UART FIFO without blocking.
 * @param[in] pUser Unused; pass nullptr.
 * @param[in] pBytes Readable frame bytes; null is rejected.
 * @param[in] length Requested byte count.
 * @return Bytes accepted by the FIFO; KymoCore sends the rest later.
 */
/*******************************************************************************
 * writeBytes
 ******************************************************************************/
static uint8_t writeBytes(void *pUser, const uint8_t *pBytes, uint8_t length)
{
    uint8_t result = 0U;
    (void)pUser;
    if (pBytes != nullptr)
    {
        int written = uart_tx_chars(kPort, reinterpret_cast<const char *>(pBytes), length);
        if (written > 0)
        {
            result = static_cast<uint8_t>(written);
        }
    }
    return result;
}

/** @brief Configure UART0 for 115200 baud, 8N1, without a TX ring buffer.
 * @return ESP_OK on success, otherwise the first driver error.
 */
/*******************************************************************************
 * initUart
 ******************************************************************************/
static esp_err_t initUart()
{
    uart_config_t uartConfig = {};
    uartConfig.baud_rate = 115200;
    uartConfig.data_bits = UART_DATA_8_BITS;
    uartConfig.parity = UART_PARITY_DISABLE;
    uartConfig.stop_bits = UART_STOP_BITS_1;
    uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uartConfig.source_clk = UART_SCLK_DEFAULT;
    esp_err_t result = uart_driver_install(kPort, 256, 0, 0, nullptr, 0);
    if (result == ESP_OK)
    {
        result = uart_param_config(kPort, &uartConfig);
    }
    return result;
}

/** @brief Stream both channels until the configuration or transport fails.
 * @details Main is called again while it sends frames; when nothing is due or the
 * FIFO is full, the task sleeps one tick so the idle task can feed the watchdog.
 */
/*******************************************************************************
 * app_main
 ******************************************************************************/
extern "C" void app_main(void)
{
    KymoStatus status = KYMO_BAD_CONFIG;
    KymoConfig config = {}; /* copied by Kymo_Init; may be local */
    config.channels = sChannels;
    config.channel_count = KYMO_COUNT_OF(sChannels);
    config.clock_ms = readClockMs;
    config.sample = readSample;
    config.write = writeBytes;
    esp_log_level_set("*", ESP_LOG_NONE);
    if (initUart() == ESP_OK)
    {
        status = Kymo_Init(&sContext, &config);
    }
    while ((status != KYMO_BAD_CONFIG) && (status != KYMO_IO_ERROR))
    {
        status = Kymo_Main(&sContext);
        if (status != KYMO_OK)
        {
            vTaskDelay(1);
        }
    }
}
