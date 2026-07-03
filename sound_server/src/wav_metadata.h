#pragma once

#include <Arduino.h>
#include <SD.h>

namespace sound_server {

struct WavMetadata {
  bool riffOk = false;
  bool waveOk = false;
  bool fmtFound = false;
  bool dataFound = false;
  uint16_t audioFormat = 0;
  uint16_t channels = 0;
  uint32_t sampleRate = 0;
  uint16_t bitsPerSample = 0;
  uint32_t dataOffset = 0;
  uint32_t dataSize = 0;
};

WavMetadata inspectWavFile(const String &path);
bool isKnownGoodFormat(const WavMetadata &meta);
void printWavMetadata(uint8_t id, const String &filename, const WavMetadata &meta);

}  // namespace sound_server
