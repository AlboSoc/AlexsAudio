#pragma once

#include <Arduino.h>

#include "packet_sender.h"

namespace trigger_client {

class UartSender : public PacketSender {
 public:
  bool begin() override;
  void printBanner() const override;
  bool sendPacket(const alexs_audio::PlaySoundPacket &packet) override;
};

}  // namespace trigger_client
