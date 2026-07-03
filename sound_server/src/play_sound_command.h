#pragma once

#include <Arduino.h>

namespace sound_server {

struct PlaySoundCommand {
  uint8_t soundId = 0;
};

}  // namespace sound_server
