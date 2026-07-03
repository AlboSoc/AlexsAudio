#pragma once

#include <Arduino.h>

#include "audio_engine.h"
#include "play_sound_packet.h"

namespace sound_server {

class UartTriggerReceiver {
 public:
  explicit UartTriggerReceiver(AudioEngine &audioEngine);

  void begin();
  void printConfig() const;
  void poll();

  AudioEngine &audioEngine_;
  PlaySoundPacketParser packetParser_;
};

}  // namespace sound_server
