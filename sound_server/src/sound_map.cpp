#include "sound_map.h"

#include <SD.h>

namespace sound_server {

int parseSoundId(const char *filename) {
  int id = 0;
  int index = 0;

  if (!isdigit(static_cast<unsigned char>(filename[index]))) {
    return -1;
  }

  while (isdigit(static_cast<unsigned char>(filename[index]))) {
    id = (id * 10) + (filename[index] - '0');
    index++;
  }

  if (filename[index] != '-') {
    return -1;
  }

  if (id < 0 || id > MAX_SOUND_ID) {
    return -1;
  }

  return id;
}

void clearSoundMap(SoundMap &soundMap) {
  for (uint8_t i = 0; i <= MAX_SOUND_ID; ++i) {
    soundMap.files[i] = "";
    soundMap.metadata[i] = WavMetadata{};
  }
}

void scanSoundFiles(SoundMap &soundMap) {
  clearSoundMap(soundMap);

  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("Could not open SD root directory.");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      const char *name = file.name();
      int id = parseSoundId(name);
      if (id >= 0) {
        soundMap.files[id] = String(name);
        String path = "/";
        path += name;
        soundMap.metadata[id] = inspectWavFile(path);
      }
    }
    file = root.openNextFile();
  }
}

void listSoundMap(const SoundMap &soundMap) {
  Serial.println("Discovered sound map:");
  for (uint8_t i = 0; i <= MAX_SOUND_ID; ++i) {
    if (soundMap.files[i].length() == 0) {
      Serial.printf("  %u -> <none>\n", i);
    } else {
      printWavMetadata(i, soundMap.files[i], soundMap.metadata[i]);
    }
  }
}

}  // namespace sound_server
