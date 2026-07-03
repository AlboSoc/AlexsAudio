#pragma once

#include <alexs_audio/play_sound_protocol.h>

namespace sound_server {

using alexs_audio::PLAY_SOUND_PACKET_MAGIC;
using alexs_audio::PLAY_SOUND_PACKET_SIZE;
using alexs_audio::PLAY_SOUND_PACKET_VERSION;
using alexs_audio::PlaySoundPacket;
using alexs_audio::PlaySoundPacketCommand;
using alexs_audio::PlaySoundPacketParser;
using alexs_audio::computePlaySoundPacketChecksum;
using alexs_audio::finalizePlaySoundPacket;
using alexs_audio::isKnownPlaySoundPacketCommand;
using alexs_audio::isValidPlaySoundPacket;

}  // namespace sound_server
