#pragma once

#include <Arduino.h>

#include <alexs_audio/command_stream.h>
#include <alexs_audio/play_sound_protocol.h>

#include "packet_sender.h"

namespace uart_trigger_client {

class HostInterface {
 public:
  explicit HostInterface(PacketSender &packetSender);

  void printHelp() const;
  void poll();

 private:
  void handleCommand(String command);
  void handlePlayCommand(const String &command);
  void handlePacket(const alexs_audio::PlaySoundPacket &packet);
  void logPacket(const char *prefix, const alexs_audio::PlaySoundPacket &packet) const;

  PacketSender &packetSender_;
  alexs_audio::CommandStream commandStream_;
};

}  // namespace uart_trigger_client
