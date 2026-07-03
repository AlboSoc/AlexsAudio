#include <Arduino.h>

#include "client_config.h"
#include "host_interface.h"
#include "uart_sender.h"

namespace {

uart_trigger_client::UartSender uartSender;
uart_trigger_client::HostInterface hostInterface(uartSender);

}  // namespace

void setup() {
  Serial.begin(uart_trigger_client::HOST_SERIAL_BAUD);
  delay(500);

  uartSender.begin();
  uartSender.printBanner();
  hostInterface.printHelp();
}

void loop() {
  hostInterface.poll();
  delay(2);
}
