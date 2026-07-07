#include <Arduino.h>

#include "button_trigger.h"
#include "client_config.h"
#include "esp_now_sender.h"
#include "host_interface.h"
#include "packet_sender.h"
#include "uart_sender.h"

namespace {

trigger_client::UartSender uartSender;
trigger_client::EspNowSender espNowSender;
trigger_client::PacketSender *packetSender = nullptr;
trigger_client::HostInterface *hostInterface = nullptr;
trigger_client::ButtonTrigger *buttonTrigger = nullptr;

}  // namespace

void setup() {
  Serial.begin(trigger_client::HOST_SERIAL_BAUD);
  delay(500);

  packetSender = trigger_client::USE_ESP_NOW
                     ? static_cast<trigger_client::PacketSender *>(&espNowSender)
                     : static_cast<trigger_client::PacketSender *>(&uartSender);
  hostInterface = new trigger_client::HostInterface(*packetSender);
  buttonTrigger = new trigger_client::ButtonTrigger(*packetSender);

  if (!packetSender->begin()) {
    Serial.println("Trigger sender begin failed.");
  }
  packetSender->printBanner();
  hostInterface->printHelp();
  buttonTrigger->begin();
  buttonTrigger->printConfig();
}

void loop() {
  if (hostInterface != nullptr) {
    hostInterface->poll();
  }
  if (buttonTrigger != nullptr) {
    buttonTrigger->poll();
  }
  delay(2);
}
