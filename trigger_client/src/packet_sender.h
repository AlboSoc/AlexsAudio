#pragma once

#include <Arduino.h>

#include <alexs_audio/play_sound_protocol.h>

namespace trigger_client {

class PacketSender {
 public:
  virtual ~PacketSender() = default;

  virtual bool begin() = 0;
  virtual void printBanner() const = 0;
  virtual bool sendPacket(const alexs_audio::PlaySoundPacket &packet) = 0;

  bool sendPlaySound(uint8_t soundId);
  bool sendStop();
  bool sendPing();

 protected:
  alexs_audio::PlaySoundPacket makePacket(alexs_audio::PlaySoundPacketCommand command,
                                          uint8_t soundId);

 private:
  uint8_t sequence_ = 0;
};

}  // namespace trigger_client
