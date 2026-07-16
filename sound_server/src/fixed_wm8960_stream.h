#pragma once

#include "AudioTools.h"
#include "AudioTools/AudioLibs/WM8960Stream.h"

namespace sound_server {

class PersistentWm8960Stream : public audio_tools::WM8960Stream {
 public:
  bool beginPersistent(audio_tools::WM8960Config startupConfig,
                       audio_tools::WM8960Config runtimeConfig) {
    (void)startupConfig;
    cfg = runtimeConfig;
    currentOutputVolume_ = runtimeConfig.default_volume;
    if (!audio_tools::WM8960Stream::begin(runtimeConfig)) {
      return false;
    }

    fixedInfo_.copyFrom(runtimeConfig);
    fixedInfoValid_ = true;
    return true;
  }

  void setAudioInfo(audio_tools::AudioInfo info) override {
    if (!fixedInfoValid_) {
      cfg.copyFrom(info);
      cfg.default_volume = currentOutputVolume_;
      audio_tools::WM8960Stream::begin(cfg);
      return;
    }

    if (info.sample_rate != fixedInfo_.sample_rate ||
        info.channels != fixedInfo_.channels ||
        info.bits_per_sample != fixedInfo_.bits_per_sample) {
      cfg.copyFrom(info);
      cfg.default_volume = currentOutputVolume_;
      audio_tools::WM8960Stream::begin(cfg);
      return;
    }

    cfg.copyFrom(fixedInfo_);
    AudioStream::setAudioInfo(fixedInfo_);
    i2s.setAudioInfo(fixedInfo_);
  }

  void setPersistentOutputVolume(float vol) {
    currentOutputVolume_ = vol;
    cfg.default_volume = vol;
    setVolumeOut(vol);
  }

 private:
  audio_tools::AudioInfo fixedInfo_;
  bool fixedInfoValid_ = false;
  float currentOutputVolume_ = 0.6f;
};

}  // namespace sound_server
