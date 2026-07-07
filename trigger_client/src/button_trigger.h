#pragma once

#include <Arduino.h>

#include "client_config.h"
#include "packet_sender.h"

namespace trigger_client {

class ButtonTrigger {
 public:
  explicit ButtonTrigger(PacketSender &packetSender);

  void begin();
  void printConfig() const;
  void poll();

 private:
  struct ButtonState {
    uint8_t pin = 0;
    uint8_t soundId = 0;
    bool lastRawPressed = false;
    bool stablePressed = false;
    uint32_t lastChangeMs = 0;
  };

  void handlePressed(const ButtonState &button);
  void updatePulse(uint8_t pin, bool &active, uint32_t &deadlineMs);
  void pulseMarker(uint8_t pin, bool &active, uint32_t &deadlineMs);

  PacketSender &packetSender_;
  ButtonState buttons_[BUTTON_COUNT];
  bool detectPulseActive_ = false;
  bool sendPulseActive_ = false;
  uint32_t detectPulseDeadlineMs_ = 0;
  uint32_t sendPulseDeadlineMs_ = 0;
};

}  // namespace trigger_client
