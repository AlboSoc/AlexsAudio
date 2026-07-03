#include "uart_trigger_receiver.h"

#include "sound_server_config.h"

namespace sound_server {

UartTriggerReceiver::UartTriggerReceiver(AudioEngine &audioEngine)
    : audioEngine_(audioEngine) {}

void UartTriggerReceiver::begin() {
  Serial2.begin(TRIGGER_UART_BAUD, SERIAL_8N1, TRIGGER_UART_RX_PIN, TRIGGER_UART_TX_PIN);
}

void UartTriggerReceiver::printConfig() const {
  Serial.printf("Trigger UART RX : GPIO %u\n", TRIGGER_UART_RX_PIN);
  Serial.printf("Trigger UART TX : GPIO %u\n", TRIGGER_UART_TX_PIN);
  Serial.printf("Trigger UART    : %lu 8-N-1\n", static_cast<unsigned long>(TRIGGER_UART_BAUD));
}

void UartTriggerReceiver::handlePacket(const PlaySoundPacket &packet) {
  if (packet.command == static_cast<uint8_t>(PlaySoundPacketCommand::Play)) {
    PlaySoundCommand playCommand;
    playCommand.soundId = packet.soundId;
    if (audioEngine_.executePlaySoundCommand(playCommand)) {
      Serial.printf("UART play sound=%u seq=%u\n", packet.soundId, packet.sequence);
    }
    return;
  }

  if (packet.command == static_cast<uint8_t>(PlaySoundPacketCommand::Stop)) {
    audioEngine_.stopCurrentPlayback(true);
    Serial.printf("UART stop seq=%u\n", packet.sequence);
    return;
  }

  if (packet.command == static_cast<uint8_t>(PlaySoundPacketCommand::Ping)) {
    Serial.printf("UART ping seq=%u\n", packet.sequence);
  }
}

void UartTriggerReceiver::poll() {
  while (Serial2.available()) {
    PlaySoundPacket packet;
    PlaySoundPacketParser::Result result =
        packetParser_.pushByte(static_cast<uint8_t>(Serial2.read()), packet);
    if (result == PlaySoundPacketParser::Result::Complete) {
      handlePacket(packet);
    } else if (result == PlaySoundPacketParser::Result::Invalid) {
      Serial.println("Discarded invalid UART trigger packet.");
    }
  }
}

}  // namespace sound_server
