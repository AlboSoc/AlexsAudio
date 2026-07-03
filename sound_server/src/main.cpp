#include <Arduino.h>

#include "audio_engine.h"
#include "serial_interface.h"
#include "sound_map.h"

namespace {

sound_server::SoundMap *soundMap = nullptr;
sound_server::AudioEngine *audioEngine = nullptr;
sound_server::SerialInterface *serialInterface = nullptr;

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);

  // Build the heavier audio objects after Arduino runtime startup to avoid
  // early static-initialization crashes inside audio-tools.
  soundMap = new sound_server::SoundMap();
  audioEngine = new sound_server::AudioEngine();
  serialInterface = new sound_server::SerialInterface(*audioEngine, *soundMap);

  audioEngine->printBanner();

  if (!audioEngine->beginSdCard()) {
    Serial.println("Check SD wiring, card formatting, and power.");
  } else {
    Serial.println("SD initialization succeeded.");
    sound_server::scanSoundFiles(*soundMap);
    sound_server::listSoundMap(*soundMap);
  }

  audioEngine->setSoundMap(soundMap);

  if (!audioEngine->beginAudio()) {
    Serial.println("Check WM8960 wiring and power.");
  } else {
    Serial.println("WM8960 initialization succeeded.");
  }

  serialInterface->printHelp();
}

void loop() {
  if (serialInterface != nullptr) {
    serialInterface->poll();
  }

  if (audioEngine != nullptr) {
    audioEngine->update();
  } else {
    delay(5);
  }
}
