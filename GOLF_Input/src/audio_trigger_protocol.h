#pragma once

#include <Arduino.h>

namespace golf_input {

constexpr uint8_t PLAY_SOUND_PACKET_MAGIC = 0xA5;
constexpr uint8_t PLAY_SOUND_PACKET_VERSION = 1;

enum class PlaySoundPacketCommand : uint8_t {
  Play = 1,
  Stop = 2,
  Ping = 3,
};

struct PlaySoundPacket {
  uint8_t magic = PLAY_SOUND_PACKET_MAGIC;
  uint8_t version = PLAY_SOUND_PACKET_VERSION;
  uint8_t command = static_cast<uint8_t>(PlaySoundPacketCommand::Play);
  uint8_t soundId = 0;
  uint8_t sequence = 0;
  uint8_t checksum = 0;
};

inline uint8_t computePlaySoundPacketChecksum(const PlaySoundPacket &packet) {
  return packet.magic ^ packet.version ^ packet.command ^ packet.soundId ^
         packet.sequence;
}

inline void finalizePlaySoundPacket(PlaySoundPacket &packet) {
  packet.checksum = computePlaySoundPacketChecksum(packet);
}

}  // namespace golf_input
