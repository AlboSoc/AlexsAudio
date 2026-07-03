#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <cstring>

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include "AudioTools/AudioLibs/WM8960Stream.h"

namespace {

constexpr uint8_t WM8960_SDA_PIN = 22;
constexpr uint8_t WM8960_SCL_PIN = 21;
constexpr uint8_t WM8960_BCLK_PIN = 27;
constexpr uint8_t WM8960_WS_PIN = 26;
constexpr uint8_t WM8960_DATA_PIN = 25;

constexpr uint8_t SD_CS_PIN = 5;
constexpr uint8_t SD_SCK_PIN = 18;
constexpr uint8_t SD_MISO_PIN = 19;
constexpr uint8_t SD_MOSI_PIN = 23;

constexpr uint8_t MAX_SOUND_ID = 7;
constexpr uint32_t SD_SPI_FREQUENCY_HZ = 1000000;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint8_t CHANNELS = 2;
constexpr uint8_t BITS_PER_SAMPLE = 16;
constexpr float DEFAULT_OUTPUT_VOLUME = 0.9f;

SPIClass sdSpi(VSPI);

WM8960Stream audioOut;
VolumeStream volumeOut(audioOut);
SineWaveGenerator<int16_t> sineWave(32000);
GeneratedSoundStream<int16_t> toneStream(sineWave);
StreamCopy toneCopier(audioOut, toneStream);
WAVDecoder wavDecoder;
EncodedAudioStream wavStream(&volumeOut, &wavDecoder);
StreamCopy wavCopier;

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

String soundFiles[MAX_SOUND_ID + 1];
WavMetadata soundMetadata[MAX_SOUND_ID + 1];
bool toneEnabled = false;
bool playbackActive = false;
int activePlaybackId = -1;
File playbackFile;
String serialBuffer;
float outputVolume = DEFAULT_OUTPUT_VOLUME;

void printBanner() {
  Serial.println();
  Serial.println("=== AlexsAudio Sound Server Bring-Up ===");
  Serial.printf("WM8960 I2C SDA : GPIO %u\n", WM8960_SDA_PIN);
  Serial.printf("WM8960 I2C SCL : GPIO %u\n", WM8960_SCL_PIN);
  Serial.printf("WM8960 I2S BCLK: GPIO %u\n", WM8960_BCLK_PIN);
  Serial.printf("WM8960 I2S WS  : GPIO %u\n", WM8960_WS_PIN);
  Serial.printf("WM8960 I2S DATA: GPIO %u\n", WM8960_DATA_PIN);
  Serial.printf("SD CS          : GPIO %u\n", SD_CS_PIN);
  Serial.printf("SD SCK         : GPIO %u\n", SD_SCK_PIN);
  Serial.printf("SD MISO        : GPIO %u\n", SD_MISO_PIN);
  Serial.printf("SD MOSI        : GPIO %u\n", SD_MOSI_PIN);
}

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

void clearSoundMap() {
  for (uint8_t i = 0; i <= MAX_SOUND_ID; ++i) {
    soundFiles[i] = "";
    soundMetadata[i] = WavMetadata{};
  }
}

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
         meta.sampleRate == 22050 && meta.bitsPerSample == 16;
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

void listSoundMap() {
  Serial.println("Discovered sound map:");
  for (uint8_t i = 0; i <= MAX_SOUND_ID; ++i) {
    if (soundFiles[i].length() == 0) {
      Serial.printf("  %u -> <none>\n", i);
    } else {
      printWavMetadata(i, soundFiles[i], soundMetadata[i]);
    }
  }
}

bool beginSdCard() {
  sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, sdSpi, SD_SPI_FREQUENCY_HZ)) {
    Serial.println("SD.begin() failed.");
    return false;
  }
  return true;
}

void scanSoundFiles() {
  clearSoundMap();

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
        soundFiles[id] = String(name);
        String path = "/";
        path += name;
        soundMetadata[id] = inspectWavFile(path);
      }
    }
    file = root.openNextFile();
  }
}

bool beginAudio() {
  Wire.begin(WM8960_SDA_PIN, WM8960_SCL_PIN);
  Wire.setClock(1000);

  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  auto config = audioOut.defaultConfig(TX_MODE);
  config.sample_rate = SAMPLE_RATE;
  config.channels = CHANNELS;
  config.bits_per_sample = BITS_PER_SAMPLE;
  config.default_volume = DEFAULT_OUTPUT_VOLUME;
  config.wire = &Wire;
  config.pin_bck = WM8960_BCLK_PIN;
  config.pin_ws = WM8960_WS_PIN;
  config.pin_data = WM8960_DATA_PIN;

  if (!audioOut.begin(config)) {
    Serial.println("WM8960 begin failed.");
    return false;
  }

  sineWave.begin(CHANNELS, SAMPLE_RATE, N_B4);
  volumeOut.setVolume(1.0);
  volumeOut.begin(config);
  audioOut.setVolumeOut(outputVolume);
  return true;
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  help      - show this help");
  Serial.println("  list      - show discovered sound IDs and WAV formats");
  Serial.println("  tone on   - enable WM8960 test tone");
  Serial.println("  tone off  - disable WM8960 test tone");
  Serial.println("  play <id> - play the WAV file mapped to sound ID");
  Serial.println("  volume <0-100> - set WM8960 output volume percentage");
}

void setOutputVolumePercent(float percent) {
  if (percent < 0.0f) {
    percent = 0.0f;
  } else if (percent > 100.0f) {
    percent = 100.0f;
  }

  outputVolume = percent / 100.0f;
  audioOut.setVolumeOut(outputVolume);
  Serial.printf("Output volume set to %.0f%%\n", percent);
}

void stopPlayback(bool printMessage) {
  if (!playbackActive) {
    return;
  }

  wavCopier.end();
  wavStream.end();
  playbackFile.close();
  playbackActive = false;

  if (printMessage) {
    if (activePlaybackId >= 0) {
      Serial.printf("Playback finished for sound ID %d\n", activePlaybackId);
    } else {
      Serial.println("Playback stopped.");
    }
  }

  activePlaybackId = -1;
}

bool startPlayback(uint8_t id) {
  if (soundFiles[id].length() == 0) {
    Serial.printf("No file mapped for sound ID %u\n", id);
    return false;
  }

  const WavMetadata &meta = soundMetadata[id];
  if (meta.fmtFound) {
    Serial.printf("WAV format for sound ID %u: fmt=%u, %lu Hz, %u ch, %u-bit%s\n",
                  id,
                  meta.audioFormat,
                  static_cast<unsigned long>(meta.sampleRate),
                  meta.channels,
                  meta.bitsPerSample,
                  meta.dataFound ? "" : ", missing data chunk");
  }

  stopPlayback(false);
  toneEnabled = false;

  String path = "/";
  path += soundFiles[id];

  playbackFile = SD.open(path.c_str(), FILE_READ);
  if (!playbackFile) {
    Serial.printf("Failed to open %s\n", path.c_str());
    return false;
  }

  if (!wavStream.begin()) {
    Serial.println("Failed to start WAV decoder.");
    playbackFile.close();
    return false;
  }

  wavCopier.begin(wavStream, playbackFile);
  playbackActive = true;
  activePlaybackId = id;

  Serial.printf("Playing sound ID %u -> %s\n", id, soundFiles[id].c_str());
  return true;
}

void handlePlayCommand(const String &command) {
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

  startPlayback(static_cast<uint8_t>(id));
}

void handleVolumeCommand(const String &command) {
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex < 0) {
    Serial.println("Usage: volume <0-100>");
    return;
  }

  float percent = command.substring(spaceIndex + 1).toFloat();
  setOutputVolumePercent(percent);
}

void handleCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command.length() == 0) {
    return;
  }

  if (command == "help") {
    printHelp();
  } else if (command == "list") {
    listSoundMap();
  } else if (command == "tone on") {
    toneEnabled = true;
    Serial.println("Tone enabled.");
  } else if (command == "tone off") {
    toneEnabled = false;
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

void pollSerial() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      handleCommand(serialBuffer);
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);

  printBanner();

  if (!beginSdCard()) {
    Serial.println("Check SD wiring, card formatting, and power.");
  } else {
    Serial.println("SD initialization succeeded.");
    scanSoundFiles();
    listSoundMap();
  }

  if (!beginAudio()) {
    Serial.println("Check WM8960 wiring and power.");
  } else {
    Serial.println("WM8960 initialization succeeded.");
  }

  printHelp();
}

void loop() {
  pollSerial();

  if (playbackActive) {
    if (!wavCopier.copy()) {
      stopPlayback(true);
    }
  } else if (toneEnabled) {
    toneCopier.copy();
  } else {
    delay(5);
  }
}
