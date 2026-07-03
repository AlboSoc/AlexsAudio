#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "packet_sender.h"

namespace trigger_client {

class EspNowSender : public PacketSender {
 public:
  bool begin() override;
  void printBanner() const override;
  bool sendPacket(const alexs_audio::PlaySoundPacket &packet) override;

 private:
  static void onPacketSent(const uint8_t *macAddr, esp_now_send_status_t status);
  bool addBroadcastPeer();
};

}  // namespace trigger_client
