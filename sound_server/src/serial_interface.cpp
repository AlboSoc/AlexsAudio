#include "serial_interface.h"

#include "play_sound_command.h"
#include "sound_server_config.h"

namespace sound_server {

SerialInterface::SerialInterface(AudioEngine &audioEngine, const SoundMap &soundMap)
    : audioEngine_(audioEngine), soundMap_(soundMap) {}

void SerialInterface::printHelp() const {
  Serial.println("Commands:");
  Serial.println("  help      - show this help");
  Serial.println("  list      - show discovered sound IDs and WAV formats");
  Serial.println("  tone on   - enable WM8960 test tone");
  Serial.println("  tone off  - disable WM8960 test tone");
  Serial.println("  play <id> - play the WAV file mapped to sound ID");
  Serial.println("  volume <0-100> - set WM8960 output volume percentage");
}

void SerialInterface::handlePlayCommand(const String &command) {
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex < 0) {
    Serial.println("Usage: play <id>");
    return;
  }

  int id = command.substring(spaceIndex + 1).toInt();
  if (id < 0 || id > MAX_SOUND_ID) {
    Serial.printf("Sound ID must be in the range 0..%u\n", MAX_SOUND_ID);
    return;
  }

  PlaySoundCommand playCommand;
  playCommand.soundId = static_cast<uint8_t>(id);
  (void)audioEngine_.executePlaySoundCommand(playCommand);
}

void SerialInterface::handleVolumeCommand(const String &command) {
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex < 0) {
    Serial.println("Usage: volume <0-100>");
    return;
  }

  float percent = command.substring(spaceIndex + 1).toFloat();
  audioEngine_.setOutputVolumePercent(percent);
}

void SerialInterface::handleCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command.length() == 0) {
    return;
  }

  if (command == "help") {
    printHelp();
  } else if (command == "list") {
    listSoundMap(soundMap_);
  } else if (command == "tone on") {
    audioEngine_.setToneEnabled(true);
    Serial.println("Tone enabled.");
  } else if (command == "tone off") {
    audioEngine_.setToneEnabled(false);
    Serial.println("Tone disabled.");
  } else if (command.startsWith("play ")) {
    handlePlayCommand(command);
  } else if (command.startsWith("volume ")) {
    handleVolumeCommand(command);
  } else {
    Serial.printf("Unknown command: %s\n", command.c_str());
    printHelp();
  }
}

void SerialInterface::poll() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      handleCommand(serialBuffer_);
      serialBuffer_ = "";
    } else {
      serialBuffer_ += c;
    }
  }
}

}  // namespace sound_server
