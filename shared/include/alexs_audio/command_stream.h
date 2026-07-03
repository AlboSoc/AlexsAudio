#pragma once

#include <Arduino.h>

#include "alexs_audio/play_sound_protocol.h"

namespace alexs_audio {

class CommandStream {
 public:
  template <typename PacketHandler, typename TextHandler, typename InvalidHandler>
  void poll(Stream &stream,
            PacketHandler onPacket,
            TextHandler onTextLine,
            InvalidHandler onInvalidPacket) {
    while (stream.available()) {
      uint8_t byte = static_cast<uint8_t>(stream.read());
      if (packetParser_.isReceiving() || byte == PLAY_SOUND_PACKET_MAGIC) {
        PlaySoundPacket packet;
        PlaySoundPacketParser::Result result = packetParser_.pushByte(byte, packet);
        if (result == PlaySoundPacketParser::Result::Complete) {
          onPacket(packet);
        } else if (result == PlaySoundPacketParser::Result::Invalid) {
          onInvalidPacket();
        }
        continue;
      }

      handleIncomingTextByte(static_cast<char>(byte), onTextLine);
    }
  }

 private:
  template <typename TextHandler>
  void handleIncomingTextByte(char c, TextHandler onTextLine) {
    if (c == '\r') {
      return;
    }
    if (c == '\n') {
      onTextLine(textBuffer_);
      textBuffer_ = "";
    } else {
      textBuffer_ += c;
    }
  }

  PlaySoundPacketParser packetParser_;
  String textBuffer_;
};

}  // namespace alexs_audio
