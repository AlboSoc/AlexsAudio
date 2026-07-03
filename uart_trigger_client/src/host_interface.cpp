#include "host_interface.h"

namespace uart_trigger_client {

HostInterface::HostInterface(PacketSender &packetSender) : packetSender_(packetSender) {}

void HostInterface::printHelp() const {
  Serial.println("Commands:");
  Serial.println("  help      - show this help");
  Serial.println("  play <id> - send a play trigger packet over wired UART");
  Serial.println("  stop      - send a stop trigger packet over wired UART");
  Serial.println("  ping      - send a ping trigger packet over wired UART");
  Serial.printf("Binary trigger packets are also accepted on this port: magic=0x%02X, version=%u, size=%u bytes\n",
                alexs_audio::PLAY_SOUND_PACKET_MAGIC,
                alexs_audio::PLAY_SOUND_PACKET_VERSION,
                static_cast<unsigned>(alexs_audio::PLAY_SOUND_PACKET_SIZE));
}

void HostInterface::logPacket(const char *prefix, const alexs_audio::PlaySoundPacket &packet) const {
  Serial.printf("%s cmd=%u sound=%u seq=%u checksum=0x%02X\n",
                prefix,
                packet.command,
                packet.soundId,
                packet.sequence,
                packet.checksum);
}

void HostInterface::handlePlayCommand(const String &command) {
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex < 0) {
    Serial.println("Usage: play <id>");
    return;
  }

  int id = command.substring(spaceIndex + 1).toInt();
  if (id < 0 || id > 255) {
    Serial.println("Sound ID must be in the range 0..255");
    return;
  }

  if (packetSender_.sendPlaySound(static_cast<uint8_t>(id))) {
    Serial.printf("CLI play sound=%d\n", id);
  } else {
    Serial.println("Failed to send play packet.");
  }
}

void HostInterface::handleCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command.length() == 0) {
    return;
  }

  if (command == "help") {
    printHelp();
  } else if (command.startsWith("play ")) {
    handlePlayCommand(command);
  } else if (command == "stop") {
    if (packetSender_.sendStop()) {
      Serial.println("CLI stop");
    } else {
      Serial.println("Failed to send stop packet.");
    }
  } else if (command == "ping") {
    if (packetSender_.sendPing()) {
      Serial.println("CLI ping");
    } else {
      Serial.println("Failed to send ping packet.");
    }
  } else {
    Serial.printf("Unknown command: %s\n", command.c_str());
    printHelp();
  }
}

void HostInterface::handlePacket(const alexs_audio::PlaySoundPacket &packet) {
  if (packetSender_.sendPacket(packet)) {
    logPacket("FORWARDED", packet);
  } else {
    Serial.println("Failed to forward trigger packet.");
  }
}

void HostInterface::poll() {
  commandStream_.poll(
      Serial,
      [this](const alexs_audio::PlaySoundPacket &packet) { handlePacket(packet); },
      [this](String command) { handleCommand(command); },
      []() { Serial.println("Discarded invalid trigger packet."); });
}

}  // namespace uart_trigger_client
