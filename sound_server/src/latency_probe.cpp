#include "latency_probe.h"

#include "sound_server_config.h"

namespace sound_server::latency_probe {
namespace {

bool rxPulseActive = false;
bool audioPulseActive = false;
uint32_t rxPulseDeadlineMs = 0;
uint32_t audioPulseDeadlineMs = 0;
uint32_t lastPacketMicros = 0;
uint8_t lastSoundId = 0;
uint8_t lastSequence = 0;
const char *lastTransportLabel = "-";
bool haveActivePlayPacket = false;

void updatePulse(uint8_t pin, bool &active, uint32_t &deadlineMs) {
  if (active && static_cast<int32_t>(millis() - deadlineMs) >= 0) {
    digitalWrite(pin, LOW);
    active = false;
  }
}

void pulsePin(uint8_t pin, bool &active, uint32_t &deadlineMs) {
  if (!ENABLE_LATENCY_MARKER_PINS) {
    return;
  }

  digitalWrite(pin, HIGH);
  active = true;
  deadlineMs = millis() + LATENCY_MARKER_PULSE_MS;
}

}  // namespace

void begin() {
  if (!ENABLE_LATENCY_MARKER_PINS) {
    return;
  }

  pinMode(LATENCY_RX_MARKER_PIN, OUTPUT);
  pinMode(LATENCY_FIRST_AUDIO_MARKER_PIN, OUTPUT);
  digitalWrite(LATENCY_RX_MARKER_PIN, LOW);
  digitalWrite(LATENCY_FIRST_AUDIO_MARKER_PIN, LOW);
}

void poll() {
  if (!ENABLE_LATENCY_MARKER_PINS) {
    return;
  }

  updatePulse(LATENCY_RX_MARKER_PIN, rxPulseActive, rxPulseDeadlineMs);
  updatePulse(LATENCY_FIRST_AUDIO_MARKER_PIN, audioPulseActive, audioPulseDeadlineMs);
}

void onPacketReceived(const char *transportLabel,
                      const alexs_audio::PlaySoundPacket &packet) {
  lastPacketMicros = micros();
  lastSoundId = packet.soundId;
  lastSequence = packet.sequence;
  lastTransportLabel = transportLabel;
  haveActivePlayPacket =
      packet.command == static_cast<uint8_t>(alexs_audio::PlaySoundPacketCommand::Play);

  pulsePin(LATENCY_RX_MARKER_PIN, rxPulseActive, rxPulseDeadlineMs);

  if (ENABLE_LATENCY_DEBUG_LOGS) {
    Serial.printf("LATENCY rx transport=%s cmd=%u sound=%u seq=%u t_us=%lu\n",
                  transportLabel,
                  packet.command,
                  packet.soundId,
                  packet.sequence,
                  static_cast<unsigned long>(lastPacketMicros));
  }
}

void onPlaybackArmed(uint8_t soundId,
                     uint32_t openFileUs,
                     uint32_t decoderBeginUs,
                     uint32_t totalPrepareUs) {
  if (!haveActivePlayPacket || soundId != lastSoundId) {
    if (ENABLE_LATENCY_DEBUG_LOGS) {
      Serial.printf("LATENCY playback-armed sound=%u without matching play packet\n", soundId);
    }
    return;
  }

  const uint32_t nowMicros = micros();
  if (ENABLE_LATENCY_DEBUG_LOGS) {
    Serial.printf(
        "LATENCY playback-armed transport=%s sound=%u seq=%u from_rx_us=%lu open_us=%lu decoder_us=%lu total_prepare_us=%lu\n",
        lastTransportLabel,
        soundId,
        lastSequence,
        static_cast<unsigned long>(nowMicros - lastPacketMicros),
        static_cast<unsigned long>(openFileUs),
        static_cast<unsigned long>(decoderBeginUs),
        static_cast<unsigned long>(totalPrepareUs));
  }
}

void onFirstAudioCopyStart(uint8_t soundId) {
  if (!haveActivePlayPacket || soundId != lastSoundId) {
    if (ENABLE_LATENCY_DEBUG_LOGS) {
      Serial.printf("LATENCY first-copy-start sound=%u without matching play packet\n", soundId);
    }
    return;
  }

  pulsePin(LATENCY_FIRST_AUDIO_MARKER_PIN, audioPulseActive, audioPulseDeadlineMs);

  const uint32_t nowMicros = micros();
  if (ENABLE_LATENCY_DEBUG_LOGS) {
    Serial.printf("LATENCY first-copy-start transport=%s sound=%u seq=%u from_rx_us=%lu\n",
                  lastTransportLabel,
                  soundId,
                  lastSequence,
                  static_cast<unsigned long>(nowMicros - lastPacketMicros));
  }
}

void onFirstAudioCopyEnd(uint8_t soundId, uint32_t copyCallUs, bool copyOk) {
  if (!haveActivePlayPacket || soundId != lastSoundId) {
    if (ENABLE_LATENCY_DEBUG_LOGS) {
      Serial.printf("LATENCY first-copy-end sound=%u without matching play packet\n", soundId);
    }
    return;
  }

  const uint32_t nowMicros = micros();
  if (ENABLE_LATENCY_DEBUG_LOGS) {
    Serial.printf(
        "LATENCY first-copy-end transport=%s sound=%u seq=%u from_rx_us=%lu copy_call_us=%lu result=%s\n",
        lastTransportLabel,
        soundId,
        lastSequence,
        static_cast<unsigned long>(nowMicros - lastPacketMicros),
        static_cast<unsigned long>(copyCallUs),
        copyOk ? "more" : "eof");
  }
}

}  // namespace sound_server::latency_probe
