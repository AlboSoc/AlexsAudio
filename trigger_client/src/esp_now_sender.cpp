#include "esp_now_sender.h"

#include <cstdio>
#include <cstring>

#include "client_config.h"

namespace trigger_client {

namespace {

constexpr uint8_t BROADCAST_MAC[ESP_NOW_ETH_ALEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

}

bool EspNowSender::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed.");
    return false;
  }

  if (esp_now_register_send_cb(onPacketSent) != ESP_OK) {
    Serial.println("ESP-NOW send callback registration failed.");
    return false;
  }

  return addBroadcastPeer();
}

void EspNowSender::printBanner() const {
  Serial.println();
  Serial.println("=== AlexsAudio ESP-NOW Trigger Client ===");
  Serial.printf("Host serial     : %lu 8-N-1\n", static_cast<unsigned long>(HOST_SERIAL_BAUD));
  Serial.printf("ESP-NOW channel : %u\n", ESP_NOW_CHANNEL);
  Serial.printf("STA MAC         : %s\n", WiFi.macAddress().c_str());
}

bool EspNowSender::addBroadcastPeer() {
  esp_now_peer_info_t peerInfo = {};
  std::memcpy(peerInfo.peer_addr, BROADCAST_MAC, ESP_NOW_ETH_ALEN);
  peerInfo.channel = ESP_NOW_CHANNEL;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);
  if (result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST) {
    return true;
  }

  Serial.printf("ESP-NOW add peer failed: %d\n", static_cast<int>(result));
  return false;
}

bool EspNowSender::sendPacket(const alexs_audio::PlaySoundPacket &packet) {
  esp_err_t result = esp_now_send(BROADCAST_MAC,
                                  reinterpret_cast<const uint8_t *>(&packet),
                                  sizeof(packet));
  return result == ESP_OK;
}

void EspNowSender::onPacketSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  char macString[18];
  std::snprintf(macString,
                sizeof(macString),
                "%02X:%02X:%02X:%02X:%02X:%02X",
                macAddr[0],
                macAddr[1],
                macAddr[2],
                macAddr[3],
                macAddr[4],
                macAddr[5]);
  Serial.printf("ESP-NOW send to %s: %s\n",
                macString,
                status == ESP_NOW_SEND_SUCCESS ? "ok" : "fail");
}

}  // namespace trigger_client
