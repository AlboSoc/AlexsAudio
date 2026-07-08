#include "wav_metadata.h"

#include <cstring>

namespace sound_server {

namespace {

uint16_t readLe16(File &file) {
  uint8_t bytes[2];
  if (file.read(bytes, sizeof(bytes)) != sizeof(bytes)) {
    return 0;
  }
  return static_cast<uint16_t>(bytes[0]) |
         (static_cast<uint16_t>(bytes[1]) << 8);
}

uint32_t readLe32(File &file) {
  uint8_t bytes[4];
  if (file.read(bytes, sizeof(bytes)) != sizeof(bytes)) {
    return 0;
  }
  return static_cast<uint32_t>(bytes[0]) |
         (static_cast<uint32_t>(bytes[1]) << 8) |
         (static_cast<uint32_t>(bytes[2]) << 16) |
         (static_cast<uint32_t>(bytes[3]) << 24);
}

bool readFourCc(File &file, char out[5]) {
  if (file.read(reinterpret_cast<uint8_t *>(out), 4) != 4) {
    return false;
  }
  out[4] = '\0';
  return true;
}

}  // namespace

WavMetadata inspectWavFile(const String &path) {
  WavMetadata meta;
  File file = SD.open(path.c_str(), FILE_READ);
  if (!file) {
    return meta;
  }

  char chunkId[5];
  char waveId[5];

  if (!readFourCc(file, chunkId)) {
    file.close();
    return meta;
  }
  meta.riffOk = std::strcmp(chunkId, "RIFF") == 0;

  (void)readLe32(file);

  if (!readFourCc(file, waveId)) {
    file.close();
    return meta;
  }
  meta.waveOk = std::strcmp(waveId, "WAVE") == 0;

  while (file.available()) {
    char subchunkId[5];
    if (!readFourCc(file, subchunkId)) {
      break;
    }

    uint32_t subchunkSize = readLe32(file);
    uint32_t dataStart = static_cast<uint32_t>(file.position());

    if (std::strcmp(subchunkId, "fmt ") == 0) {
      meta.fmtFound = true;
      meta.audioFormat = readLe16(file);
      meta.channels = readLe16(file);
      meta.sampleRate = readLe32(file);
      (void)readLe32(file);
      (void)readLe16(file);
      meta.bitsPerSample = readLe16(file);
    } else if (std::strcmp(subchunkId, "data") == 0) {
      meta.dataFound = true;
      meta.dataOffset = dataStart;
      meta.dataSize = subchunkSize;
      break;
    }

    uint32_t nextChunk = dataStart + subchunkSize + (subchunkSize & 1U);
    if (!file.seek(nextChunk)) {
      break;
    }
  }

  file.close();
  return meta;
}

bool isKnownGoodFormat(const WavMetadata &meta) {
  return meta.riffOk && meta.waveOk && meta.fmtFound && meta.dataFound &&
         meta.audioFormat == 1 && meta.channels == 2 &&
         meta.sampleRate == 44100 && meta.bitsPerSample == 16;
}

void printWavMetadata(uint8_t id, const String &filename, const WavMetadata &meta) {
  Serial.printf("  %u -> %s", id, filename.c_str());

  if (!meta.riffOk || !meta.waveOk) {
    Serial.println(" [not a RIFF/WAVE file]");
    return;
  }

  if (!meta.fmtFound) {
    Serial.println(" [missing fmt chunk]");
    return;
  }

  Serial.printf(" [fmt=%u, %lu Hz, %u ch, %u-bit",
                meta.audioFormat,
                static_cast<unsigned long>(meta.sampleRate),
                meta.channels,
                meta.bitsPerSample);

  if (meta.dataFound) {
    Serial.printf(", data@%lu", static_cast<unsigned long>(meta.dataOffset));
  } else {
    Serial.print(", missing data chunk");
  }

  if (isKnownGoodFormat(meta)) {
    Serial.print(", known-good");
  }

  Serial.println("]");
}

}  // namespace sound_server
