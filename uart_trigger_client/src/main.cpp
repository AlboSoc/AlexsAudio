#include <Arduino.h>

#include "client_config.h"
#include "esp_now_sender.h"
#include "host_interface.h"
#include "packet_sender.h"
#include "uart_sender.h"

namespace {

uart_trigger_client::UartSender uartSender;
uart_trigger_client::EspNowSender espNowSender;
uart_trigger_client::PacketSender *packetSender = nullptr;
uart_trigger_client::HostInterface *hostInterface = nullptr;

}  // namespace

void setup() {
  Serial.begin(uart_trigger_client::HOST_SERIAL_BAUD);
  delay(500);

  packetSender = uart_trigger_client::USE_ESP_NOW
                     ? static_cast<uart_trigger_client::PacketSender *>(&espNowSender)
                     : static_cast<uart_trigger_client::PacketSender *>(&uartSender);
  hostInterface = new uart_trigger_client::HostInterface(*packetSender);

  if (!packetSender->begin()) {
    Serial.println("Trigger sender begin failed.");
  }
  packetSender->printBanner();
  hostInterface->printHelp();
}

void loop() {
  if (hostInterface != nullptr) {
    hostInterface->poll();
  }
  delay(2);
}
