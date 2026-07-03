#pragma once

#include <Arduino.h>
#include <alexs_audio/command_stream.h>

#include "audio_engine.h"
#include "play_sound_packet.h"
#include "sound_map.h"

namespace sound_server {

class SerialInterface {
 public:
  SerialInterface(AudioEngine &audioEngine, const SoundMap &soundMap);

 void printHelp() const;
  void poll();

 private:
  void handleCommand(String command);
  void handlePacket(const PlaySoundPacket &packet);
  void handlePlayCommand(const String &command);
  void handleVolumeCommand(const String &command);

  AudioEngine &audioEngine_;
  const SoundMap &soundMap_;
  alexs_audio::CommandStream commandStream_;
};

}  // namespace sound_server
