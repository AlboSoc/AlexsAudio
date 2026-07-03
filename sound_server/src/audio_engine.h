#pragma once

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include "AudioTools/AudioLibs/WM8960Stream.h"

#include "play_sound_command.h"
#include "sound_map.h"

namespace sound_server {

class AudioEngine {
 public:
  bool beginSdCard();
  bool beginAudio();

  void setSoundMap(const SoundMap *soundMap);
  void printBanner() const;
  void setOutputVolumePercent(float percent);
  bool executePlaySoundCommand(const PlaySoundCommand &command);
  void setToneEnabled(bool enabled);
  bool isToneEnabled() const;
  void update();

 private:
  bool validatePlaySoundCommand(const PlaySoundCommand &command) const;
  bool startPlayback(uint8_t id);
  void stopPlayback(bool printMessage);

  SPIClass sdSpi_{VSPI};
  WM8960Stream audioOut_;
  VolumeStream volumeOut_{audioOut_};
  SineWaveGenerator<int16_t> sineWave_{32000};
  GeneratedSoundStream<int16_t> toneStream_{sineWave_};
  StreamCopy toneCopier_{audioOut_, toneStream_};
  WAVDecoder wavDecoder_;
  EncodedAudioStream wavStream_{&volumeOut_, &wavDecoder_};
  StreamCopy wavCopier_;
  File playbackFile_;

  const SoundMap *soundMap_ = nullptr;
  bool toneEnabled_ = false;
  bool playbackActive_ = false;
  int activePlaybackId_ = -1;
  float outputVolume_ = DEFAULT_OUTPUT_VOLUME;
};

}  // namespace sound_server
