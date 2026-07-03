#pragma once

#include <Arduino.h>

#include "audio_engine.h"
#include "play_sound_command.h"
#include "play_sound_packet.h"

namespace sound_server {

void executeTriggerPacket(AudioEngine &audioEngine,
                          const PlaySoundPacket &packet,
                          const char *transportLabel);

}  // namespace sound_server
