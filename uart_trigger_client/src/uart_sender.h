#pragma once

#include <Arduino.h>

#include <alexs_audio/play_sound_protocol.h>

namespace uart_trigger_client {

class UartSender {
 public:
  void begin();
  void printBanner() const;

  bool sendPacket(const alexs_audio::PlaySoundPacket &packet);
  bool sendPlaySound(uint8_t soundId);
  bool sendStop();
  bool sendPing();

 private:
  alexs_audio::PlaySoundPacket makePacket(alexs_audio::PlaySoundPacketCommand command,
                                          uint8_t soundId);

  uint8_t sequence_ = 0;
};

}  // namespace uart_trigger_client
