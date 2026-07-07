#pragma once

#include <Arduino.h>

namespace sound_server {

constexpr uint8_t WM8960_SDA_PIN = 22;
constexpr uint8_t WM8960_SCL_PIN = 21;
constexpr uint8_t WM8960_BCLK_PIN = 27;
constexpr uint8_t WM8960_WS_PIN = 26;
constexpr uint8_t WM8960_DATA_PIN = 25;

constexpr uint8_t SD_CS_PIN = 5;
constexpr uint8_t SD_SCK_PIN = 18;
constexpr uint8_t SD_MISO_PIN = 19;
constexpr uint8_t SD_MOSI_PIN = 23;

constexpr uint8_t MAX_SOUND_ID = 7;
constexpr uint32_t SD_SPI_FREQUENCY_HZ = 1000000;
constexpr uint32_t WM8960_STARTUP_SAMPLE_RATE = 44100;
constexpr uint32_t SAMPLE_RATE = 22050;
constexpr uint8_t CHANNELS = 2;
constexpr uint8_t BITS_PER_SAMPLE = 16;
constexpr float DEFAULT_OUTPUT_VOLUME = 0.5f;

constexpr uint8_t TRIGGER_UART_RX_PIN = 16;
constexpr uint8_t TRIGGER_UART_TX_PIN = 17;
constexpr uint32_t TRIGGER_UART_BAUD = 115200;
constexpr uint8_t ESP_NOW_CHANNEL = 1;
constexpr bool ENABLE_TRIGGER_UART = false;

constexpr bool ENABLE_LATENCY_MARKER_PINS = true;
constexpr uint8_t LATENCY_RX_MARKER_PIN = 32;
constexpr uint8_t LATENCY_FIRST_AUDIO_MARKER_PIN = 33;
constexpr uint16_t LATENCY_MARKER_PULSE_MS = 2;

}  // namespace sound_server
