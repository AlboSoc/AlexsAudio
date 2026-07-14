#include "button_trigger.h"

#include "client_config.h"

namespace trigger_client {

ButtonTrigger::ButtonTrigger(PacketSender &packetSender) : packetSender_(packetSender) {
  for (size_t i = 0; i < BUTTON_COUNT; ++i) {
    buttons_[i].pin = BUTTON_PINS[i];
    buttons_[i].soundId = BUTTON_SOUND_IDS[i];
  }
}

void ButtonTrigger::begin() {
  for (size_t i = 0; i < BUTTON_COUNT; ++i) {
    pinMode(buttons_[i].pin, INPUT_PULLUP);
    bool pressed = digitalRead(buttons_[i].pin) == LOW;
    buttons_[i].lastRawPressed = pressed;
    buttons_[i].stablePressed = pressed;
    buttons_[i].lastChangeMs = millis();
  }

  if (ENABLE_LATENCY_MARKER_PINS) {
    pinMode(BUTTON_DETECT_MARKER_PIN, OUTPUT);
    pinMode(SEND_COMPLETE_MARKER_PIN, OUTPUT);
    digitalWrite(BUTTON_DETECT_MARKER_PIN, LOW);
    digitalWrite(SEND_COMPLETE_MARKER_PIN, LOW);
  }
}

void ButtonTrigger::printConfig() const {
  if (!ENABLE_BUTTON_INPUT) {
    return;
  }

  Serial.println("Button trigger input enabled:");
  for (size_t i = 0; i < BUTTON_COUNT; ++i) {
    Serial.printf("  GPIO %u -> sound %u\n", buttons_[i].pin, buttons_[i].soundId);
  }
  Serial.printf("Debounce        : %u ms\n", static_cast<unsigned>(BUTTON_DEBOUNCE_MS));

  if (ENABLE_LATENCY_MARKER_PINS) {
    Serial.printf("Marker detect   : GPIO %u\n", BUTTON_DETECT_MARKER_PIN);
    Serial.printf("Marker sent     : GPIO %u\n", SEND_COMPLETE_MARKER_PIN);
  }
}

void ButtonTrigger::handlePressed(const ButtonState &button) {
  const uint32_t detectMicros = micros();
  pulseMarker(BUTTON_DETECT_MARKER_PIN, detectPulseActive_, detectPulseDeadlineMs_);
  bool ok = packetSender_.sendPlaySound(button.soundId+4);
  const uint32_t sentMicros = micros();
  pulseMarker(SEND_COMPLETE_MARKER_PIN, sendPulseActive_, sendPulseDeadlineMs_);

  Serial.printf("BUTTON play sound=%u pin=%u detect_us=%lu send_call_us=%lu result=%s\n",
                button.soundId,
                button.pin,
                static_cast<unsigned long>(detectMicros),
                static_cast<unsigned long>(sentMicros - detectMicros),
                ok ? "ok" : "fail");
}

void ButtonTrigger::updatePulse(uint8_t pin, bool &active, uint32_t &deadlineMs) {
  if (active && static_cast<int32_t>(millis() - deadlineMs) >= 0) {
    digitalWrite(pin, LOW);
    active = false;
  }
}

void ButtonTrigger::pulseMarker(uint8_t pin, bool &active, uint32_t &deadlineMs) {
  if (!ENABLE_LATENCY_MARKER_PINS) {
    return;
  }

  digitalWrite(pin, HIGH);
  active = true;
  deadlineMs = millis() + MARKER_PULSE_MS;
}

void ButtonTrigger::poll() {
  if (!ENABLE_BUTTON_INPUT) {
    return;
  }

  updatePulse(BUTTON_DETECT_MARKER_PIN, detectPulseActive_, detectPulseDeadlineMs_);
  updatePulse(SEND_COMPLETE_MARKER_PIN, sendPulseActive_, sendPulseDeadlineMs_);

  const uint32_t nowMs = millis();
  for (size_t i = 0; i < BUTTON_COUNT; ++i) {
    ButtonState &button = buttons_[i];
    bool rawPressed = digitalRead(button.pin) == LOW;

    if (rawPressed != button.lastRawPressed) {
      button.lastRawPressed = rawPressed;
      button.lastChangeMs = nowMs;
    }

    if ((nowMs - button.lastChangeMs) < BUTTON_DEBOUNCE_MS) {
      continue;
    }

    if (button.stablePressed == rawPressed) {
      continue;
    }

    button.stablePressed = rawPressed;
    if (button.stablePressed) {
      handlePressed(button);
    }
  }
}

}  // namespace trigger_client
