#include "uart_sender.h"

#include "client_config.h"

namespace uart_trigger_client {

void UartSender::begin() {
  Serial2.begin(TRIGGER_UART_BAUD, SERIAL_8N1, TRIGGER_UART_RX_PIN, TRIGGER_UART_TX_PIN);
}

void UartSender::printBanner() const {
  Serial.println();
  Serial.println("=== AlexsAudio UART Trigger Client ===");
  Serial.printf("Host serial     : %lu 8-N-1\n", static_cast<unsigned long>(HOST_SERIAL_BAUD));
  Serial.printf("Trigger UART RX : GPIO %u\n", TRIGGER_UART_RX_PIN);
  Serial.printf("Trigger UART TX : GPIO %u\n", TRIGGER_UART_TX_PIN);
  Serial.printf("Trigger UART    : %lu 8-N-1\n", static_cast<unsigned long>(TRIGGER_UART_BAUD));
}

bool UartSender::sendPacket(const alexs_audio::PlaySoundPacket &packet) {
  return Serial2.write(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet)) ==
         sizeof(packet);
}

alexs_audio::PlaySoundPacket UartSender::makePacket(
    alexs_audio::PlaySoundPacketCommand command,
    uint8_t soundId) {
  alexs_audio::PlaySoundPacket packet;
  packet.command = static_cast<uint8_t>(command);
  packet.soundId = soundId;
  packet.sequence = sequence_++;
  alexs_audio::finalizePlaySoundPacket(packet);
  return packet;
}

bool UartSender::sendPlaySound(uint8_t soundId) {
  return sendPacket(makePacket(alexs_audio::PlaySoundPacketCommand::Play, soundId));
}

bool UartSender::sendStop() {
  return sendPacket(makePacket(alexs_audio::PlaySoundPacketCommand::Stop, 0));
}

bool UartSender::sendPing() {
  return sendPacket(makePacket(alexs_audio::PlaySoundPacketCommand::Ping, 0));
}

}  // namespace uart_trigger_client
