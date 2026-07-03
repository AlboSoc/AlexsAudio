#pragma once

#include <Arduino.h>

namespace uart_trigger_client {

constexpr uint32_t HOST_SERIAL_BAUD = 115200;
constexpr uint8_t TRIGGER_UART_RX_PIN = 16;
constexpr uint8_t TRIGGER_UART_TX_PIN = 17;
constexpr uint32_t TRIGGER_UART_BAUD = 115200;

}  // namespace uart_trigger_client
