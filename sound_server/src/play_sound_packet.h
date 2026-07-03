#pragma once

#include <Arduino.h>

namespace sound_server {

constexpr uint8_t PLAY_SOUND_PACKET_MAGIC = 0xA5;
constexpr uint8_t PLAY_SOUND_PACKET_VERSION = 1;
constexpr size_t PLAY_SOUND_PACKET_SIZE = 6;

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

inline bool isKnownPlaySoundPacketCommand(uint8_t command) {
  return command == static_cast<uint8_t>(PlaySoundPacketCommand::Play) ||
         command == static_cast<uint8_t>(PlaySoundPacketCommand::Stop) ||
         command == static_cast<uint8_t>(PlaySoundPacketCommand::Ping);
}

inline bool isValidPlaySoundPacket(const PlaySoundPacket &packet) {
  return packet.magic == PLAY_SOUND_PACKET_MAGIC &&
         packet.version == PLAY_SOUND_PACKET_VERSION &&
         isKnownPlaySoundPacketCommand(packet.command) &&
         packet.checksum == computePlaySoundPacketChecksum(packet);
}

class PlaySoundPacketParser {
 public:
  enum class Result {
    None,
    Complete,
    Invalid,
  };

  bool isReceiving() const { return index_ > 0; }

  Result pushByte(uint8_t byte, PlaySoundPacket &packet) {
    if (index_ == 0 && byte != PLAY_SOUND_PACKET_MAGIC) {
      return Result::None;
    }

    bytes_[index_++] = byte;
    if (index_ < PLAY_SOUND_PACKET_SIZE) {
      return Result::None;
    }

    packet.magic = bytes_[0];
    packet.version = bytes_[1];
    packet.command = bytes_[2];
    packet.soundId = bytes_[3];
    packet.sequence = bytes_[4];
    packet.checksum = bytes_[5];
    index_ = 0;

    if (!isValidPlaySoundPacket(packet)) {
      return Result::Invalid;
    }

    return Result::Complete;
  }

 private:
  uint8_t bytes_[PLAY_SOUND_PACKET_SIZE] = {0};
  size_t index_ = 0;
};

}  // namespace sound_server
