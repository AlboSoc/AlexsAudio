#pragma once

#include <Arduino.h>

#include "sound_server_config.h"
#include "wav_metadata.h"

namespace sound_server {

struct SoundMap {
  String files[MAX_SOUND_ID + 1];
  WavMetadata metadata[MAX_SOUND_ID + 1];
};

int parseSoundId(const char *filename);
void clearSoundMap(SoundMap &soundMap);
void scanSoundFiles(SoundMap &soundMap);
void listSoundMap(const SoundMap &soundMap);

}  // namespace sound_server
