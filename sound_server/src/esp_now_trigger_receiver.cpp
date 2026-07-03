#include "esp_now_trigger_receiver.h"

#include <cstdio>
#include <cstring>

#include "packet_executor.h"
#include "sound_server_config.h"

namespace sound_server {

EspNowTriggerReceiver *EspNowTriggerReceiver::instance_ = nullptr;

EspNowTriggerReceiver::EspNowTriggerReceiver(AudioEngine &audioEngine)
    : audioEngine_(audioEngine) {}

bool EspNowTriggerReceiver::begin() {
  instance_ = this;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed.");
    return false;
  }

  if (esp_now_register_recv_cb(onDataReceived) != ESP_OK) {
    Serial.println("ESP-NOW receive callback registration failed.");
    return false;
  }

  return true;
}

void EspNowTriggerReceiver::printConfig() const {
  Serial.printf("ESP-NOW channel : %u\n", ESP_NOW_CHANNEL);
  Serial.printf("ESP-NOW STA MAC : %s\n", WiFi.macAddress().c_str());
}

void EspNowTriggerReceiver::onDataReceived(const uint8_t *macAddr,
                                           const uint8_t *data,
                                           int dataLen) {
  if (instance_ == nullptr || dataLen != static_cast<int>(sizeof(PlaySoundPacket))) {
    return;
  }

  PlaySoundPacket packet;
  std::memcpy(&packet, data, sizeof(packet));
  if (!isValidPlaySoundPacket(packet)) {
    return;
  }

  instance_->queuePacket(macAddr, packet);
}

void EspNowTriggerReceiver::queuePacket(const uint8_t *macAddr, const PlaySoundPacket &packet) {
  portENTER_CRITICAL(&pendingMux_);
  std::memcpy(pendingMac_, macAddr, sizeof(pendingMac_));
  pendingPacket_ = packet;
  hasPendingPacket_ = true;
  portEXIT_CRITICAL(&pendingMux_);
}

void EspNowTriggerReceiver::poll() {
  bool hasPacket = false;
  PlaySoundPacket packet;
  uint8_t macAddr[ESP_NOW_ETH_ALEN] = {0};

  portENTER_CRITICAL(&pendingMux_);
  if (hasPendingPacket_) {
    packet = pendingPacket_;
    std::memcpy(macAddr, pendingMac_, sizeof(macAddr));
    hasPendingPacket_ = false;
    hasPacket = true;
  }
  portEXIT_CRITICAL(&pendingMux_);

  if (!hasPacket) {
    return;
  }

  char macString[18];
  std::snprintf(macString,
                sizeof(macString),
                "%02X:%02X:%02X:%02X:%02X:%02X",
                macAddr[0],
                macAddr[1],
                macAddr[2],
                macAddr[3],
                macAddr[4],
                macAddr[5]);
  Serial.printf("ESP-NOW recv from %s\n", macString);
  executeTriggerPacket(audioEngine_, packet, "ESP-NOW");
}

}  // namespace sound_server
