#pragma once

#include <Arduino.h>

namespace trigger_client {

constexpr uint32_t HOST_SERIAL_BAUD = 115200;
constexpr bool USE_ESP_NOW = true;
constexpr uint8_t TRIGGER_UART_RX_PIN = 16;
constexpr uint8_t TRIGGER_UART_TX_PIN = 17;
constexpr uint32_t TRIGGER_UART_BAUD = 115200;
constexpr uint8_t ESP_NOW_CHANNEL = 1;

}  // namespace trigger_client
