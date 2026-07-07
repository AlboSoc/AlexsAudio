#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

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

constexpr uint32_t SD_SPI_FREQUENCY_HZ = 1000000;
constexpr uint32_t FIXED_SAMPLE_RATE = 22050;
constexpr uint32_t LEGACY_SAMPLE_RATE = 44100;
constexpr uint8_t CHANNELS = 2;
constexpr uint8_t BITS_PER_SAMPLE = 16;
constexpr float DEFAULT_OUTPUT_VOLUME = 0.5f;

enum class AudioMode {
  Fixed22050,
  Reinit22050,
  Legacy44100,
};

class BringupWm8960Stream : public audio_tools::WM8960Stream {
 public:
  bool beginBringup(audio_tools::WM8960Config config, bool fixedMode) {
    fixedMode_ = fixedMode;
    cfg = config;
    fixedInfo_.copyFrom(config);
    fixedInfoValid_ = true;
    return audio_tools::WM8960Stream::begin(config);
  }

  void setFixedMode(bool enabled) {
    fixedMode_ = enabled;
  }

  bool fixedMode() const {
    return fixedMode_;
  }

  void setPersistentOutputVolume(float vol) {
    cfg.default_volume = vol;
    setVolumeOut(vol);
  }

  void setAudioInfo(audio_tools::AudioInfo info) override {
    if (!fixedMode_ || !fixedInfoValid_) {
      audio_tools::WM8960Stream::setAudioInfo(info);
      return;
    }

    if (info.sample_rate != fixedInfo_.sample_rate ||
        info.channels != fixedInfo_.channels ||
        info.bits_per_sample != fixedInfo_.bits_per_sample) {
      Serial.printf(
          "Ignoring runtime audio change: got %lu Hz, %u ch, %u-bit; keeping %lu Hz, %u ch, %u-bit\n",
          static_cast<unsigned long>(info.sample_rate),
          info.channels,
          info.bits_per_sample,
          static_cast<unsigned long>(fixedInfo_.sample_rate),
          fixedInfo_.channels,
          fixedInfo_.bits_per_sample);
      return;
    }

    cfg.copyFrom(fixedInfo_);
    AudioStream::setAudioInfo(fixedInfo_);
    i2s.setAudioInfo(fixedInfo_);
  }

 private:
  audio_tools::AudioInfo fixedInfo_;
  bool fixedInfoValid_ = false;
  bool fixedMode_ = true;
};

SPIClass sdSpi(VSPI);
BringupWm8960Stream audioOut;
audio_tools::WAVDecoder wavDecoder;
audio_tools::EncodedAudioStream wavStream(&audioOut, &wavDecoder);
audio_tools::StreamCopy wavCopier;
audio_tools::SineWaveGenerator<int16_t> sineWave(8000);
audio_tools::GeneratedSoundStream<int16_t> toneStream(sineWave);
audio_tools::StreamCopy toneCopier(audioOut, toneStream);
File playbackFile;
String currentFileName;
bool playbackActive = false;
bool toneEnabled = false;
float outputVolume = DEFAULT_OUTPUT_VOLUME;
AudioMode currentMode = AudioMode::Fixed22050;
bool audioHardwareActive = false;

const char *modeName(AudioMode mode) {
  switch (mode) {
    case AudioMode::Fixed22050:
      return "fixed-22050";
    case AudioMode::Reinit22050:
      return "reinit-22050";
    case AudioMode::Legacy44100:
      return "legacy-44100";
  }
  return "unknown";
}

uint32_t baseSampleRate(AudioMode mode) {
  return mode == AudioMode::Legacy44100 ? LEGACY_SAMPLE_RATE : FIXED_SAMPLE_RATE;
}

bool modeUsesFixedInit(AudioMode mode) {
  return mode == AudioMode::Fixed22050;
}

audio_tools::WM8960Config makeAudioConfig(uint32_t sampleRate) {
  auto config = audioOut.defaultConfig(TX_MODE);
  config.sample_rate = sampleRate;
  config.channels = CHANNELS;
  config.bits_per_sample = BITS_PER_SAMPLE;
  config.default_volume = DEFAULT_OUTPUT_VOLUME;
  config.wire = &Wire;
  config.pin_bck = WM8960_BCLK_PIN;
  config.pin_ws = WM8960_WS_PIN;
  config.pin_data = WM8960_DATA_PIN;
  return config;
}

bool reconfigureAudioHardware(AudioMode mode) {
  if (audioHardwareActive) {
    audioOut.end();
    audioHardwareActive = false;
  }
  auto config = makeAudioConfig(baseSampleRate(mode));
  if (!audioOut.beginBringup(config, modeUsesFixedInit(mode))) {
    Serial.println("WM8960 begin failed.");
    return false;
  }
  audioHardwareActive = true;
  audioOut.setPersistentOutputVolume(outputVolume);
  sineWave.begin(CHANNELS, baseSampleRate(mode), N_B4);
  currentMode = mode;
  Serial.printf("WM8960 mode active: %s\n", modeName(currentMode));
  return true;
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
    Serial.printf("Playback finished: %s\n", currentFileName.c_str());
  }
}

bool startPlayback(const String &fileName) {
  stopPlayback(false);
  toneEnabled = false;

  String path = "/";
  path += fileName;

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
  currentFileName = fileName;

  Serial.printf("Playing %s using mode=%s\n",
                currentFileName.c_str(),
                modeName(currentMode));
  return true;
}

void printRootDirectory() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory.");
    return;
  }

  Serial.println("Root directory:");
  File entry = root.openNextFile();
  while (entry) {
    Serial.printf("  %s%s\n",
                  entry.name(),
                  entry.isDirectory() ? "/" : "");
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  help          - show this help");
  Serial.println("  list          - list files on the SD card root");
  Serial.println("  play <file>   - play a WAV file from SD root");
  Serial.println("  stop          - stop current playback");
  Serial.println("  tone on       - enable WM8960 test tone");
  Serial.println("  tone off      - disable WM8960 test tone");
  Serial.println("  volume <0-100> - set WM8960 output volume percentage");
  Serial.println("  mode fixed    - keep WM8960 initialized once at 22050 Hz");
  Serial.println("  mode reinit   - allow WAV header to reinitialize WM8960 from 22050 Hz");
  Serial.println("  mode legacy   - recreate the older 44100 Hz plain WM8960 startup path");
}

void handleCommand(String command) {
  command.trim();
  if (command.length() == 0) {
    return;
  }

  if (command.equalsIgnoreCase("help")) {
    printHelp();
    return;
  }

  if (command.equalsIgnoreCase("list")) {
    printRootDirectory();
    return;
  }

  if (command.equalsIgnoreCase("stop")) {
    toneEnabled = false;
    stopPlayback(true);
    return;
  }

  if (command.equalsIgnoreCase("tone on")) {
    stopPlayback(false);
    toneEnabled = true;
    Serial.println("Tone enabled.");
    return;
  }

  if (command.equalsIgnoreCase("tone off")) {
    toneEnabled = false;
    Serial.println("Tone disabled.");
    return;
  }

  if (command.startsWith("play ")) {
    String fileName = command.substring(5);
    fileName.trim();
    if (fileName.length() == 0) {
      Serial.println("Usage: play <file>");
      return;
    }
    (void)startPlayback(fileName);
    return;
  }

  if (command.startsWith("volume ")) {
    String value = command.substring(7);
    value.trim();
    float percent = value.toFloat();
    if (percent < 0.0f) {
      percent = 0.0f;
    } else if (percent > 100.0f) {
      percent = 100.0f;
    }
    outputVolume = percent / 100.0f;
    audioOut.setPersistentOutputVolume(outputVolume);
    Serial.printf("Output volume set to %.0f%%\n", percent);
    return;
  }

  if (command.equalsIgnoreCase("mode fixed")) {
    stopPlayback(false);
    toneEnabled = false;
    (void)reconfigureAudioHardware(AudioMode::Fixed22050);
    return;
  }

  if (command.equalsIgnoreCase("mode reinit")) {
    stopPlayback(false);
    toneEnabled = false;
    (void)reconfigureAudioHardware(AudioMode::Reinit22050);
    return;
  }

  if (command.equalsIgnoreCase("mode legacy")) {
    stopPlayback(false);
    toneEnabled = false;
    (void)reconfigureAudioHardware(AudioMode::Legacy44100);
    return;
  }

  Serial.printf("Unknown command: %s\n", command.c_str());
  printHelp();
}

void pollSerialCommands() {
  static String line;
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      handleCommand(line);
      line = "";
      continue;
    }
    line += c;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("=== WM8960 WAV Bring-Up ===");

  AudioLogger::instance().begin(Serial, AudioLogger::Error);

  Wire.begin(WM8960_SDA_PIN, WM8960_SCL_PIN);
  Wire.setClock(1000);

  sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, sdSpi, SD_SPI_FREQUENCY_HZ)) {
    Serial.println("SD.begin() failed.");
  } else {
    Serial.println("SD initialization succeeded.");
  }

  if (reconfigureAudioHardware(currentMode)) {
    Serial.println("WM8960 initialization succeeded.");
  }

  Serial.printf("WM8960 I2C SDA : GPIO %u\n", WM8960_SDA_PIN);
  Serial.printf("WM8960 I2C SCL : GPIO %u\n", WM8960_SCL_PIN);
  Serial.printf("WM8960 I2S BCLK: GPIO %u\n", WM8960_BCLK_PIN);
  Serial.printf("WM8960 I2S WS  : GPIO %u\n", WM8960_WS_PIN);
  Serial.printf("WM8960 I2S DATA: GPIO %u\n", WM8960_DATA_PIN);
  Serial.printf("SD CS          : GPIO %u\n", SD_CS_PIN);
  Serial.printf("SD SCK         : GPIO %u\n", SD_SCK_PIN);
  Serial.printf("SD MISO        : GPIO %u\n", SD_MISO_PIN);
  Serial.printf("SD MOSI        : GPIO %u\n", SD_MOSI_PIN);
  Serial.printf("SD SPI         : %lu Hz\n", static_cast<unsigned long>(SD_SPI_FREQUENCY_HZ));
  Serial.printf("Fixed format   : %lu Hz, %u ch, %u-bit\n",
                static_cast<unsigned long>(FIXED_SAMPLE_RATE),
                CHANNELS,
                BITS_PER_SAMPLE);
  Serial.printf("Legacy start   : %lu Hz\n", static_cast<unsigned long>(LEGACY_SAMPLE_RATE));
  Serial.printf("Default volume : %.0f%%\n", DEFAULT_OUTPUT_VOLUME * 100.0f);
  Serial.printf("Default WM8960 mode: %s\n", modeName(currentMode));
  printHelp();
}

void loop() {
  pollSerialCommands();

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
