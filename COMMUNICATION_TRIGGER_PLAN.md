# Communication Trigger Plan

## Purpose

This note records the intended phased approach for adding remote sound triggering to the `AlexsAudio` sound-server.

The main design goal is to separate:

- what a trigger means
- how the trigger arrives

That separation should let us add and test transports incrementally without repeatedly rewriting the playback logic.

## Core Recommendation

Use a transport-neutral internal command type and make every trigger source feed the same execution path.

Recommended terminology:

- `PlaySoundCommand`
  - the internal semantic request after parsing and validation
- packet / frame / payload
  - the transport-specific encoded bytes used on `UART`, `ESP-NOW`, or any future link

This keeps the architecture cleaner than using one type for both meanings.

## High-Level Flow

Every trigger path should follow the same stages:

1. Receive transport bytes or local user input.
2. Parse into a transport-level packet or message.
3. Validate the packet contents.
4. Convert to a `PlaySoundCommand`.
5. Execute the command through one shared playback entry point.

That gives us a structure like:

```text
CLI / WebSerial / UART / ESP-NOW
            |
            v
      Transport Parser
            |
            v
     PlaySoundCommand
            |
            v
      Command Executor
            |
            v
       sound_id -> WAV
```

## Recommended Internal Interface

The sound-server should expose one shared execution path, for example:

```c
struct PlaySoundCommand {
  uint8_t sound_id;
  uint8_t flags;
  uint8_t volume_percent;
};

bool executePlaySoundCommand(const PlaySoundCommand& cmd);
```

Notes:

- `sound_id` is the only field that is definitely required for the first version.
- `flags` can be reserved for future behavior such as `stop_current`, `loop`, or `priority`.
- `volume_percent` can be omitted initially if we want the first transport packets to stay minimal.

For early bring-up, a smaller v1 is perfectly fine:

```c
struct PlaySoundCommand {
  uint8_t sound_id;
};
```

The important part is that the CLI, `UART`, and `ESP-NOW` all end up calling the same executor.

## Phased Plan

### Phase 0: Command Abstraction

First refactor the current local command-line behavior so that:

- `play 3` does not directly manipulate playback internals
- instead it builds a `PlaySoundCommand`
- then passes that command to one shared executor

This gives us:

- one canonical playback path
- less duplication
- easier future transport work

This is the preferred first coding step before adding external comms.

### Phase 1: PC-to-Sound-Server Triggering Over UART

Add a `UART` transport to the sound-server and use a simple one-page WebSerial app on the PC as the first external trigger source.

Why this is a good first transport:

- no extra hardware needed beyond the sound-server ESP32 already on the desk
- very fast feedback loop
- easy to inspect raw traffic
- easy to test packet framing and error handling

Recommended first deliverables:

- a very small `UART` receiver in the sound-server
- a minimal packet format
- a tiny browser page using WebSerial to send test packets

This is mainly a transport test harness, not the final product.

### Phase 2: Second ESP32 as a Wired UART Client

Once the PC-driven path is stable, add a second ESP32 as the first embedded trigger client.

This second board can:

- send the same logical trigger command over `UART`
- expose its own small serial CLI
- optionally also expose a simple local test interface later

This phase proves:

- board-to-board wiring
- trigger latency on embedded hardware
- reuse of the same transport contract without the browser in the loop

### Phase 3: Add ESP-NOW as an Alternative Transport

After the command contract and wired path are stable, add `ESP-NOW` support as an alternative transport.

The second ESP32 can then drive the sound-server over either:

- wired `UART`
- wireless `ESP-NOW`

This keeps `ESP-NOW` as a transport adapter, not a separate application design.

## Transport Advice

### UART First

Treat the first wired software transport simply as `UART`.

If the final hardware later needs better noise immunity or cable length:

- keep the software model as `UART`
- place `RS-485` underneath it as the physical layer

That keeps the application code stable while allowing the hardware layer to improve.

### Packet Framing

For the very earliest smoke test, a text command can be acceptable.

However, the preferred direction is to move fairly quickly to a small framed packet, even for `UART`.

A sensible future packet structure would be something like:

```c
struct PlaySoundPacketV1 {
  uint8_t magic;
  uint8_t version;
  uint8_t type;
  uint8_t length;
  uint8_t sound_id;
  uint8_t checksum;
};
```

Why that is preferable to long-term text commands:

- easier validation
- easier resynchronization on serial noise
- easier reuse over `ESP-NOW`
- easier extension later

For now, the important principle is:

- isolate the parser
- do not let transport parsing leak into playback logic

## Testing Strategy

The recommended test ladder is:

1. Local serial CLI builds `PlaySoundCommand`.
2. PC WebSerial sends trigger packets over `UART`.
3. Second ESP32 sends the same logical command over `UART`.
4. Second ESP32 sends the same logical command over `ESP-NOW`.

This progression is useful because each stage adds only one new variable.

## Why This Plan Makes Sense

This approach is intentionally conservative:

- it reuses one playback execution path
- it keeps transport concerns modular
- it gives a test harness before extra hardware is required
- it avoids designing the wireless path before the application command is stable

Most importantly, it matches the current project style:

- get something working
- keep it working
- add the next layer without destabilizing the last one

## Suggested Near-Term Coding Tasks

The next implementation steps should be:

1. Add a second ESP32 sender that can emit the same trigger packet over wired `UART`.
2. Decide whether the sender should expose a tiny local CLI, buttons, or both for bench testing.
3. Reuse the same packet contract for `ESP-NOW`.
4. Keep the browser WebSerial page as a convenient host-side test harness.

## Status

At the time of writing:

- local WAV playback from the sound-server is working
- Phase 0 is complete via a shared `PlaySoundCommand` executor path in `sound_server`
- Phase 1 is complete via packet-triggered playback over `UART` plus a browser WebSerial test page
- Phase 2 is complete via the second ESP32 trigger client and the shared packet contract
- the first Phase 3 implementation is now in place with ESP-NOW sender and receiver support
- the next major milestone is transport hardening: real bench verification, acknowledgement decisions, and any duplicate-suppression or retry policy
