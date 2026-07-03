#pragma once

#include <Arduino.h>

#include "audio_engine.h"
#include "sound_map.h"

namespace sound_server {

class SerialInterface {
 public:
  SerialInterface(AudioEngine &audioEngine, const SoundMap &soundMap);

  void printHelp() const;
  void poll();

 private:
  void handleCommand(String command);
  void handlePlayCommand(const String &command);
  void handleVolumeCommand(const String &command);

  AudioEngine &audioEngine_;
  const SoundMap &soundMap_;
  String serialBuffer_;
};

}  // namespace sound_server
