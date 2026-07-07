#include "packet_executor.h"

#include "latency_probe.h"

namespace sound_server {

void executeTriggerPacket(AudioEngine &audioEngine,
                          const PlaySoundPacket &packet,
                          const char *transportLabel) {
  latency_probe::onPacketReceived(transportLabel, packet);

  if (packet.command == static_cast<uint8_t>(PlaySoundPacketCommand::Play)) {
    PlaySoundCommand playCommand;
    playCommand.soundId = packet.soundId;
    if (audioEngine.executePlaySoundCommand(playCommand)) {
      Serial.printf("%s play sound=%u seq=%u\n",
                    transportLabel,
                    packet.soundId,
                    packet.sequence);
    }
    return;
  }

  if (packet.command == static_cast<uint8_t>(PlaySoundPacketCommand::Stop)) {
    audioEngine.stopCurrentPlayback(true);
    Serial.printf("%s stop seq=%u\n", transportLabel, packet.sequence);
    return;
  }

  if (packet.command == static_cast<uint8_t>(PlaySoundPacketCommand::Ping)) {
    Serial.printf("%s ping seq=%u\n", transportLabel, packet.sequence);
  }
}

}  // namespace sound_server
