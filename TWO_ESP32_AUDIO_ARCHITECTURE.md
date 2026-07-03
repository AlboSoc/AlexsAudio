# Two ESP32 Audio Architecture

## Goal

Support two ESP32 devices:

1. A game ESP32 that runs Alex's game and sends commands like "play sound `i`".
2. A sound-server ESP32 that receives those commands and plays audio files from an SD card.

The sound-server ESP32 is expected to use fixed-format audio files with stable naming, for example:

- `0-bird.wav`
- `1-dog.wav`
- `2-golf-swing.wav`

## Updated Requirements

The link between the two ESP32 devices should:

1. Be convenient to set up over roughly 10 m.
2. Be tolerant of EMI / electrical noise.
3. Ideally have low latency.
4. Only need to send a small integer sound ID, likely from a small set such as `0..7`.

## Chosen Direction

Support both of these transports:

- `ESP-NOW` as the normal, convenient, cable-free option.
- `UART` as the wired transport option.

For the wired case, `RS-485` should be treated as the preferred physical layer underneath `UART` when noise tolerance or cable distance matter.

The important design choice is that both transports should carry the same tiny application-level packet.

## Recommended v1 Strategy

- Keep the application protocol transport-agnostic.
- Start with manual selection of transport rather than automatic failover.
- Treat the wired option in software as `UART`; add `RS-485` transceivers at the hardware layer when needed.
- Use a small fixed sound ID range such as `0..7`.
- Use `WAV` files first for simplicity.
- On a new play request, stop the current sound and start the new one immediately.

Automatic live fallback from `ESP-NOW` to the wired `UART` path can be added later if needed, but it should not be a day-one requirement.

## System Overview

### Game ESP32

Responsibilities:

- Run the game logic.
- Decide when a sound should play.
- Send a tiny command packet over either `ESP-NOW` or `UART`.

Example API:

```c
audioClient.playSound(3);
```

### Sound-Server ESP32

Responsibilities:

- Initialize SD card storage.
- Scan the SD card for audio files with leading numeric IDs.
- Receive packets from either transport.
- Validate each packet.
- Map `sound_id` to a file.
- Trigger playback immediately.

## Packet Format

Use a single small packet format for both `ESP-NOW` and `UART`.

```c
struct PlaySoundPacket {
  uint8_t magic;      // 0xA5
  uint8_t version;    // 1
  uint8_t command;    // 1 = play, 2 = stop, 3 = ping
  uint8_t sound_id;   // 0..7 initially
  uint8_t sequence;   // increments each send
  uint8_t checksum;   // xor of previous bytes
};
```

Suggested commands:

- `1`: play sound `sound_id`
- `2`: stop current sound
- `3`: ping / health check

This format is intentionally tiny, easy to debug, and easy to send over either transport.

## File Naming and Sound Lookup

The sound server should scan the SD card at startup and build a lookup table based on the leading integer in the filename.

Examples:

- `0-bird.wav` -> `sound_id = 0`
- `1-dog.wav` -> `sound_id = 1`
- `2-golf-swing.wav` -> `sound_id = 2`

Simple rule:

- A valid sound file starts with `"<id>-"`.
- The game side sends only `sound_id`, never a filename.

## Transport Notes

### ESP-NOW

Why use it:

- No cable.
- Convenient setup.
- Usually low latency.
- Good default choice for demos and simple installations.

Planned use:

- Pair sender and receiver by MAC address.
- Send the `PlaySoundPacket` directly.
- Optionally log send success/failure and basic link health.

### UART

Why use it:

- Simple software model.
- Easy bench testing with direct serial links.
- Natural fit for tiny fixed-size packets.

Planned use:

- The software transport just reads and writes bytes on a UART.
- For short bench tests, that can be direct TTL UART.
- For the real installation, the preferred physical layer is RS-485.

Suggested starting serial configuration:

- `115200 8N1`

That is much more than enough bandwidth for this use case.

### RS-485 Physical Layer

Why use it:

- Better tolerance to electrical noise.
- Good fit for around 10 m of cable.
- More deterministic than wireless in difficult environments.

Software impact:

- Usually the application layer does not need to know RS-485 is being used.
- In most cases the wired transport can still just be treated as UART.
- The main hardware-specific detail is whether the transceiver needs explicit `DE/RE` direction control.

## Runtime Flow

### Game Side

1. A game event occurs.
2. The game decides which sound ID to play.
3. The audio client sends a `PlaySoundPacket` using the selected transport.

### Sound-Server Side

1. Transport layer receives packet bytes.
2. Packet is validated using `magic`, `version`, and `checksum`.
3. A command is pushed into a queue.
4. The audio task pops the command.
5. The requested file is played from SD card.

Using a queue between transport reception and audio playback is recommended so transport timing is decoupled from audio timing.

## Playback Policy

Recommended first behavior:

- If a new play command arrives, stop the current sound and start the new one immediately.

This keeps the first version simple and usually matches game sound-effect expectations.

Possible future extensions:

- Allow overlapping sounds.
- Reserve separate mixer channels.
- Prioritize certain sounds over others.

## Proposed Software Structure

### Shared

`packet.h`

- `PlaySoundPacket`
- checksum helper
- encode/decode helpers

### Game ESP32

`transport.h`

- common transport interface
- `sendPlaySound(sound_id)`

Implementations:

- `EspNowTransport`
- `UartTransport`

`game_audio_client.h`

- small wrapper used by game logic

### Sound-Server ESP32

`receiver.h`

- receives packets
- validates them
- pushes commands into a queue

`sound_player.h`

- scans SD card
- maps `sound_id` to file
- triggers playback

## Transport Interface

Suggested common transport interface:

```c
class IAudioTransport {
 public:
  virtual bool begin() = 0;
  virtual bool send(const PlaySoundPacket& packet) = 0;
  virtual bool available() = 0;
  virtual bool receive(PlaySoundPacket& packet) = 0;
};
```

Likely implementations:

- `EspNowTransport`
- `UartTransport`

Notes:

- `UartTransport` should not care whether the physical layer is direct TTL UART or UART-through-RS-485.
- If the RS-485 transceiver needs `DE/RE` control, that should live inside `UartTransport`.

## Sound-Server Behavior

Expected startup sequence:

1. Initialize logging / serial monitor.
2. Initialize storage.
3. Scan SD card for sound files.
4. Build `sound_id -> filename` lookup table.
5. Initialize audio output hardware.
6. Initialize selected transport.
7. Enter receive/play loop.

Expected runtime behavior:

1. Receive packet.
2. Validate packet.
3. Ignore duplicates if sequence handling is enabled.
4. Push command into queue.
5. Audio task handles playback.

Recommended first playback policy:

- `play`: stop current file and start requested file immediately
- `stop`: stop current file
- `ping`: optionally reply or just update link-health state

## Game-Side Behavior

Expected runtime behavior:

1. Game logic decides a sound event occurred.
2. Convert game event to `sound_id`.
3. Populate packet fields.
4. Increment sequence.
5. Send over selected transport.

Recommended first behavior:

- fire-and-forget transmission
- no retry logic initially
- optional logging in debug builds only

## Recommended Implementation Order

1. Build the sound-server locally first.
2. Add SD-card scanning and simple local playback testing.
3. Define the shared packet format.
4. Add `ESP-NOW`.
5. Add `UART` bench testing.
6. Add `RS-485` transceivers on the wired path.
7. Add simple startup or compile-time transport selection.

## Bring-Up Plan

### Milestone 1: Local Sound Server

Goal:

- Prove SD card lookup and audio playback without inter-device transport.

Tasks:

1. Boot sound-server ESP32.
2. Scan SD card.
3. Print discovered sound map.
4. Trigger playback from serial console or a local test button.

### Milestone 2: Shared Packet Layer

Goal:

- Finalize the packet definition and validation logic.

Tasks:

1. Implement packet struct.
2. Implement checksum helper.
3. Implement encode/decode helpers.
4. Add a simple packet test harness.

### Milestone 3: ESP-NOW Path

Goal:

- Get end-to-end wireless triggering working.

Tasks:

1. Implement sender.
2. Implement receiver.
3. Trigger sound-server playback from received packet.
4. Measure practical latency and reliability.

### Milestone 4: UART Bench Path

Goal:

- Prove the wired software transport without worrying about RS-485 hardware yet.

Tasks:

1. Implement `UartTransport`.
2. Test with direct UART connection on the bench.
3. Verify packet framing and playback.

### Milestone 5: RS-485 Physical Layer

Goal:

- Swap in RS-485 transceivers with minimal software change.

Tasks:

1. Add transceivers at both ends.
2. Add `DE/RE` control if needed.
3. Verify reliable operation over the expected cable length.
4. Compare behavior against ESP-NOW in the target environment.

## Initial Recommendations Summary

- Primary transport: `ESP-NOW`
- Wired transport: `UART`
- Preferred wired physical layer: `RS-485`
- Packet: fixed-size, tiny, transport-independent
- Sound IDs: `0..7`
- Files: `WAV`
- Transport selection: manual at first
- Playback policy: interrupt current sound and play the new one
- First wired implementation: direct UART on bench, RS-485 afterward

## Open Questions

- Which ESP32 board variant will be used on each side?
- Will the sound-server use the WM8960 path, or something simpler?
- Is overlapping sound playback needed, or is one-at-a-time enough?
- Does the game side need acknowledgement, or is fire-and-forget acceptable?
- Will the chosen RS-485 modules need explicit `DE/RE` direction control?
- Should transport selection be compile-time, startup-time, or runtime configurable?
- Does the sound-server need to remember the last packet sequence to suppress duplicates?

