#include "audio_trigger_sender.h"

#include <cstring>

#include "audio_trigger_config.h"

namespace {

constexpr uint8_t kBroadcastMac[ESP_NOW_ETH_ALEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

}

bool AudioTriggerSender::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(golf_input::AUDIO_TRIGGER_ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    return false;
  }

  ready_ = addBroadcastPeer();
  return ready_;
}

bool AudioTriggerSender::isReady() const {
  return ready_;
}

void AudioTriggerSender::printBanner() const {
  Serial.println("=== Glow Golf ESP-NOW Audio Trigger ===");
  Serial.printf("ESP-NOW channel : %u\n", golf_input::AUDIO_TRIGGER_ESP_NOW_CHANNEL);
  Serial.printf("STA MAC         : %s\n", WiFi.macAddress().c_str());
}

bool AudioTriggerSender::sendPlaySound(uint8_t soundId) {
  return sendPacket(makePacket(golf_input::PlaySoundPacketCommand::Play, soundId));
}

bool AudioTriggerSender::sendPacket(const golf_input::PlaySoundPacket &packet) {
  if (!ready_) {
    return false;
  }

  esp_err_t result = esp_now_send(
      kBroadcastMac, reinterpret_cast<const uint8_t *>(&packet), sizeof(packet));
  return result == ESP_OK;
}

bool AudioTriggerSender::addBroadcastPeer() {
  esp_now_peer_info_t peerInfo = {};
  std::memcpy(peerInfo.peer_addr, kBroadcastMac, ESP_NOW_ETH_ALEN);
  peerInfo.channel = golf_input::AUDIO_TRIGGER_ESP_NOW_CHANNEL;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);
  return result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST;
}

golf_input::PlaySoundPacket AudioTriggerSender::makePacket(
    golf_input::PlaySoundPacketCommand command,
    uint8_t soundId) {
  golf_input::PlaySoundPacket packet;
  packet.command = static_cast<uint8_t>(command);
  packet.soundId = soundId;
  packet.sequence = sequence_++;
  golf_input::finalizePlaySoundPacket(packet);
  return packet;
}
