#include "packet_sender.h"

namespace uart_trigger_client {

alexs_audio::PlaySoundPacket PacketSender::makePacket(
    alexs_audio::PlaySoundPacketCommand command,
    uint8_t soundId) {
  alexs_audio::PlaySoundPacket packet;
  packet.command = static_cast<uint8_t>(command);
  packet.soundId = soundId;
  packet.sequence = sequence_++;
  alexs_audio::finalizePlaySoundPacket(packet);
  return packet;
}

bool PacketSender::sendPlaySound(uint8_t soundId) {
  return sendPacket(makePacket(alexs_audio::PlaySoundPacketCommand::Play, soundId));
}

bool PacketSender::sendStop() {
  return sendPacket(makePacket(alexs_audio::PlaySoundPacketCommand::Stop, 0));
}

bool PacketSender::sendPing() {
  return sendPacket(makePacket(alexs_audio::PlaySoundPacketCommand::Ping, 0));
}

}  // namespace uart_trigger_client
