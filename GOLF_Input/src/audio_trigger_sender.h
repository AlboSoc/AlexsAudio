#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "audio_trigger_protocol.h"

class AudioTriggerSender {
 public:
  bool begin();
  bool isReady() const;
  void printBanner() const;
  bool sendPlaySound(uint8_t soundId);
  bool sendPacket(const golf_input::PlaySoundPacket &packet);

 private:
  bool addBroadcastPeer();
  golf_input::PlaySoundPacket makePacket(golf_input::PlaySoundPacketCommand command,
                                         uint8_t soundId);

  bool ready_ = false;
  uint8_t sequence_ = 0;
};
