#pragma once

#include <Arduino.h>

#include <alexs_audio/play_sound_protocol.h>

namespace sound_server::latency_probe {

void begin();
void poll();
void onPacketReceived(const char *transportLabel,
                      const alexs_audio::PlaySoundPacket &packet);
void onPlaybackArmed(uint8_t soundId,
                     uint32_t openFileUs,
                     uint32_t decoderBeginUs,
                     uint32_t totalPrepareUs);
void onFirstAudioCopyStart(uint8_t soundId);
void onFirstAudioCopyEnd(uint8_t soundId, uint32_t copyCallUs, bool copyOk);

}  // namespace sound_server::latency_probe
