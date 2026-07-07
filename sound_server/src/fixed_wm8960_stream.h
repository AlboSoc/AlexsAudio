#pragma once

#include "AudioTools.h"
#include "AudioTools/AudioLibs/WM8960Stream.h"

namespace sound_server {

class PersistentWm8960Stream : public audio_tools::WM8960Stream {
 public:
  bool beginPersistent(audio_tools::WM8960Config config) {
    cfg = config;
    currentOutputVolume_ = config.default_volume;
    return audio_tools::WM8960Stream::begin(config);
  }

  void setAudioInfo(audio_tools::AudioInfo info) override {
    cfg.copyFrom(info);
    cfg.default_volume = currentOutputVolume_;
    audio_tools::WM8960Stream::begin(cfg);
  }

  void setPersistentOutputVolume(float vol) {
    currentOutputVolume_ = vol;
    cfg.default_volume = vol;
    setVolumeOut(vol);
  }

 private:
  float currentOutputVolume_ = 0.6f;
};

}  // namespace sound_server
