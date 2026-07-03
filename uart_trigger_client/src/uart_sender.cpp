#include "uart_sender.h"

#include "client_config.h"

namespace uart_trigger_client {

bool UartSender::begin() {
  Serial2.begin(TRIGGER_UART_BAUD, SERIAL_8N1, TRIGGER_UART_RX_PIN, TRIGGER_UART_TX_PIN);
  return true;
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

}  // namespace uart_trigger_client
