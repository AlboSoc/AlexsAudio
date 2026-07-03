#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "audio_engine.h"
#include "play_sound_packet.h"

namespace sound_server {

class EspNowTriggerReceiver {
 public:
  explicit EspNowTriggerReceiver(AudioEngine &audioEngine);

  bool begin();
  void printConfig() const;
  void poll();

 private:
  static void onDataReceived(const uint8_t *macAddr, const uint8_t *data, int dataLen);
  void queuePacket(const uint8_t *macAddr, const PlaySoundPacket &packet);

  AudioEngine &audioEngine_;
  portMUX_TYPE pendingMux_ = portMUX_INITIALIZER_UNLOCKED;
  bool hasPendingPacket_ = false;
  PlaySoundPacket pendingPacket_ = {};
  uint8_t pendingMac_[ESP_NOW_ETH_ALEN] = {0};

  static EspNowTriggerReceiver *instance_;
};

}  // namespace sound_server
