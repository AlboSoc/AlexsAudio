#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>

#include "AudioTools.h"
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

SPIClass sdSpi(VSPI);

WM8960Stream audioOut;
SineWaveGenerator<int16_t> sineWave(32000);
GeneratedSoundStream<int16_t> toneStream(sineWave);
StreamCopy toneCopier(audioOut, toneStream);

String soundFiles[MAX_SOUND_ID + 1];
bool toneEnabled = false;
String serialBuffer;

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
  }
}

void listSoundMap() {
  Serial.println("Discovered sound map:");
  for (uint8_t i = 0; i <= MAX_SOUND_ID; ++i) {
    if (soundFiles[i].length() == 0) {
      Serial.printf("  %u -> <none>\n", i);
    } else {
      Serial.printf("  %u -> %s\n", i, soundFiles[i].c_str());
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
  config.wire = &Wire;
  config.pin_bck = WM8960_BCLK_PIN;
  config.pin_ws = WM8960_WS_PIN;
  config.pin_data = WM8960_DATA_PIN;

  if (!audioOut.begin(config)) {
    Serial.println("WM8960 begin failed.");
    return false;
  }

  sineWave.begin(CHANNELS, SAMPLE_RATE, N_B4);
  audioOut.setVolumeOut(0.5);
  return true;
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  help      - show this help");
  Serial.println("  list      - show discovered sound IDs");
  Serial.println("  tone on   - enable WM8960 test tone");
  Serial.println("  tone off  - disable WM8960 test tone");
  Serial.println("  play <id> - resolve and print the file mapped to sound ID");
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

  if (soundFiles[id].length() == 0) {
    Serial.printf("No file mapped for sound ID %d\n", id);
    return;
  }

  Serial.printf("Resolved sound ID %d -> %s\n", id, soundFiles[id].c_str());
  Serial.println("Playback from SD is the next implementation step.");
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

  if (toneEnabled) {
    toneCopier.copy();
  } else {
    delay(5);
  }
}
