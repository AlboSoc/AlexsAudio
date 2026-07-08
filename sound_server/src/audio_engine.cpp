#include "audio_engine.h"

#include "latency_probe.h"
#include "sound_server_config.h"

namespace sound_server {

namespace {
constexpr uint32_t WM8960_I2C_FREQUENCY_HZ = 100000;
}

bool AudioEngine::beginSdCard() {
  sdSpi_.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, sdSpi_, SD_SPI_FREQUENCY_HZ)) {
    Serial.println("SD.begin() failed.");
    return false;
  }
  return true;
}

bool AudioEngine::beginAudio() {
  Wire.begin(WM8960_SDA_PIN, WM8960_SCL_PIN);
  Wire.setClock(WM8960_I2C_FREQUENCY_HZ);

  AudioLogger::instance().begin(Serial, AudioLogger::Error);

  auto startupConfig = audioOut_.defaultConfig(TX_MODE);
  startupConfig.sample_rate = WM8960_STARTUP_SAMPLE_RATE;
  startupConfig.channels = CHANNELS;
  startupConfig.bits_per_sample = BITS_PER_SAMPLE;
  startupConfig.default_volume = DEFAULT_OUTPUT_VOLUME;
  startupConfig.i2c_retry_count = WM8960_I2C_RETRY_COUNT;
  startupConfig.wire = &Wire;
  startupConfig.pin_bck = WM8960_BCLK_PIN;
  startupConfig.pin_ws = WM8960_WS_PIN;
  startupConfig.pin_data = WM8960_DATA_PIN;

  auto runtimeConfig = startupConfig;
  runtimeConfig.sample_rate = SAMPLE_RATE;

  if (!audioOut_.beginPersistent(startupConfig, runtimeConfig)) {
    Serial.println("WM8960 begin failed.");
    return false;
  }

  sineWave_.begin(CHANNELS, SAMPLE_RATE, N_B4);
  audioOut_.setPersistentOutputVolume(outputVolume_);
  return true;
}

void AudioEngine::setSoundMap(const SoundMap *soundMap) {
  soundMap_ = soundMap;
}

void AudioEngine::printBanner() const {
  Serial.println();
  Serial.println("=== AlexsAudio Sound Server Bring-Up ===");
  Serial.printf("WM8960 I2C SDA : GPIO %u\n", WM8960_SDA_PIN);
  Serial.printf("WM8960 I2C SCL : GPIO %u\n", WM8960_SCL_PIN);
  Serial.printf("WM8960 I2C     : %lu Hz\n", static_cast<unsigned long>(WM8960_I2C_FREQUENCY_HZ));
  Serial.printf("WM8960 I2S BCLK: GPIO %u\n", WM8960_BCLK_PIN);
  Serial.printf("WM8960 I2S WS  : GPIO %u\n", WM8960_WS_PIN);
  Serial.printf("WM8960 I2S DATA: GPIO %u\n", WM8960_DATA_PIN);
  Serial.printf("SD CS          : GPIO %u\n", SD_CS_PIN);
  Serial.printf("SD SCK         : GPIO %u\n", SD_SCK_PIN);
  Serial.printf("SD MISO        : GPIO %u\n", SD_MISO_PIN);
  Serial.printf("SD MOSI        : GPIO %u\n", SD_MOSI_PIN);
  Serial.printf("SD SPI         : %lu Hz\n", static_cast<unsigned long>(SD_SPI_FREQUENCY_HZ));
  Serial.printf("WM8960 startup : %lu Hz, %u ch, %u-bit\n",
                static_cast<unsigned long>(WM8960_STARTUP_SAMPLE_RATE),
                CHANNELS,
                BITS_PER_SAMPLE);
  Serial.printf("WM8960 runtime : %lu Hz, %u ch, %u-bit\n",
                static_cast<unsigned long>(SAMPLE_RATE),
                CHANNELS,
                BITS_PER_SAMPLE);
  Serial.printf("WM8960 retries : %lu\n", static_cast<unsigned long>(WM8960_I2C_RETRY_COUNT));
  Serial.printf("WAV target     : %lu Hz, %u ch, %u-bit\n",
                static_cast<unsigned long>(SAMPLE_RATE),
                CHANNELS,
                BITS_PER_SAMPLE);
  Serial.printf("Trigger UART   : %s\n", ENABLE_TRIGGER_UART ? "enabled" : "disabled");
  if (ENABLE_LATENCY_MARKER_PINS) {
    Serial.printf("Latency RX pin  : GPIO %u\n", LATENCY_RX_MARKER_PIN);
    Serial.printf("Latency audio pin: GPIO %u\n", LATENCY_FIRST_AUDIO_MARKER_PIN);
  }
}

void AudioEngine::setOutputVolumePercent(float percent) {
  if (percent < 0.0f) {
    percent = 0.0f;
  } else if (percent > 100.0f) {
    percent = 100.0f;
  }

  outputVolume_ = percent / 100.0f;
  audioOut_.setPersistentOutputVolume(outputVolume_);
  Serial.printf("Output volume set to %.0f%%\n", percent);
}

bool AudioEngine::validatePlaySoundCommand(const PlaySoundCommand &command) const {
  if (command.soundId > MAX_SOUND_ID) {
    Serial.printf("Sound ID must be in the range 0..%u\n", MAX_SOUND_ID);
    return false;
  }

  if (soundMap_ == nullptr || soundMap_->files[command.soundId].length() == 0) {
    Serial.printf("No file mapped for sound ID %u\n", command.soundId);
    return false;
  }

  return true;
}

void AudioEngine::stopPlayback(bool printMessage) {
  if (!playbackActive_) {
    return;
  }

  wavCopier_.end();
  wavStream_.end();
  playbackFile_.close();
  playbackActive_ = false;
  firstAudioCopyPending_ = false;
  playbackPrimePending_ = false;

  if (printMessage) {
    if (activePlaybackId_ >= 0) {
      Serial.printf("Playback finished for sound ID %d\n", activePlaybackId_);
    } else {
      Serial.println("Playback stopped.");
    }
  }

  activePlaybackId_ = -1;
}

bool AudioEngine::startPlayback(uint8_t id) {
  if (soundMap_ == nullptr || soundMap_->files[id].length() == 0) {
    Serial.printf("No file mapped for sound ID %u\n", id);
    return false;
  }

  const WavMetadata &meta = soundMap_->metadata[id];
  if (meta.fmtFound) {
    Serial.printf("WAV format for sound ID %u: fmt=%u, %lu Hz, %u ch, %u-bit%s\n",
                  id,
                  meta.audioFormat,
                  static_cast<unsigned long>(meta.sampleRate),
                  meta.channels,
                  meta.bitsPerSample,
                  meta.dataFound ? "" : ", missing data chunk");

    if (meta.sampleRate != SAMPLE_RATE ||
        meta.channels != CHANNELS ||
        meta.bitsPerSample != BITS_PER_SAMPLE) {
      Serial.printf("Refusing sound ID %u because it is not the fixed playback format (%lu Hz, %u ch, %u-bit)\n",
                    id,
                    static_cast<unsigned long>(SAMPLE_RATE),
                    CHANNELS,
                    BITS_PER_SAMPLE);
      return false;
    }
  }

  stopPlayback(false);
  toneEnabled_ = false;

  String path = "/";
  path += soundMap_->files[id];

  const uint32_t startMicros = micros();
  playbackFile_ = SD.open(path.c_str(), FILE_READ);
  const uint32_t afterOpenMicros = micros();
  if (!playbackFile_) {
    Serial.printf("Failed to open %s\n", path.c_str());
    return false;
  }

  if (!wavStream_.begin()) {
    Serial.println("Failed to start WAV decoder.");
    playbackFile_.close();
    return false;
  }
  const uint32_t afterDecoderMicros = micros();

  wavCopier_.begin(wavStream_, playbackFile_);
  playbackActive_ = true;
  firstAudioCopyPending_ = true;
  playbackPrimePending_ = true;
  activePlaybackId_ = id;

  latency_probe::onPlaybackArmed(id,
                                 afterOpenMicros - startMicros,
                                 afterDecoderMicros - afterOpenMicros,
                                 afterDecoderMicros - startMicros);

  Serial.printf("Playing sound ID %u -> %s\n", id, soundMap_->files[id].c_str());
  return true;
}

bool AudioEngine::executePlaySoundCommand(const PlaySoundCommand &command) {
  if (!validatePlaySoundCommand(command)) {
    return false;
  }

  return startPlayback(command.soundId);
}

void AudioEngine::stopCurrentPlayback(bool printMessage) {
  toneEnabled_ = false;
  stopPlayback(printMessage);
}

void AudioEngine::setToneEnabled(bool enabled) {
  toneEnabled_ = enabled;
}

bool AudioEngine::isToneEnabled() const {
  return toneEnabled_;
}

void AudioEngine::update() {
  if (playbackActive_) {
    if (playbackPrimePending_ && activePlaybackId_ >= 0) {
      if (ENABLE_LATENCY_DEBUG_LOGS) {
        Serial.printf("LATENCY note sound=%d entering first wavCopier.copy()\n", activePlaybackId_);
      }
      playbackPrimePending_ = false;
    }

    const uint32_t copyStartMicros = micros();
    const bool measureFirstCopy = firstAudioCopyPending_ && activePlaybackId_ >= 0;
    if (measureFirstCopy) {
      latency_probe::onFirstAudioCopyStart(static_cast<uint8_t>(activePlaybackId_));
      firstAudioCopyPending_ = false;
    }
    bool copyOk = wavCopier_.copy();
    if (measureFirstCopy) {
      latency_probe::onFirstAudioCopyEnd(static_cast<uint8_t>(activePlaybackId_),
                                         micros() - copyStartMicros,
                                         copyOk);
    }
    if (!copyOk) {
      stopPlayback(true);
    }
  } else if (toneEnabled_) {
    toneCopier_.copy();
  } else {
    delay(5);
  }
}

}  // namespace sound_server
