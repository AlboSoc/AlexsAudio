#pragma once

#include <Arduino.h>

namespace trigger_client {

constexpr uint32_t HOST_SERIAL_BAUD = 115200;
constexpr bool USE_ESP_NOW = true;
constexpr uint8_t TRIGGER_UART_RX_PIN = 16;
constexpr uint8_t TRIGGER_UART_TX_PIN = 17;
constexpr uint32_t TRIGGER_UART_BAUD = 115200;
constexpr uint8_t ESP_NOW_CHANNEL = 1;

constexpr bool ENABLE_BUTTON_INPUT = true;
constexpr uint8_t BUTTON_COUNT = 4;
constexpr uint8_t BUTTON_PINS[BUTTON_COUNT] = {32, 33, 25, 26};
constexpr uint8_t BUTTON_SOUND_IDS[BUTTON_COUNT] = {0, 1, 2, 3};
constexpr uint16_t BUTTON_DEBOUNCE_MS = 20;

constexpr bool ENABLE_LATENCY_MARKER_PINS = true;
constexpr uint8_t BUTTON_DETECT_MARKER_PIN = 4;
constexpr uint8_t SEND_COMPLETE_MARKER_PIN = 13;
constexpr uint16_t MARKER_PULSE_MS = 2;

}  // namespace trigger_client
