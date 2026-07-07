# Latency Measurement Plan

This note captures the current plan for measuring the button-to-sound path in the two-ESP32 setup.

## Goal

Measure where time is being spent between:

1. a physical button press on the trigger client
2. packet transmission to the sound server
3. packet reception on the sound server
4. playback being armed from SD / WAV setup
5. first audio samples being pushed into the WM8960 path

## Current Firmware Instrumentation

## Trigger Client

The `trigger_client` firmware now supports direct button input.

Default button mapping:

- `GPIO 32 -> sound 0`
- `GPIO 33 -> sound 1`
- `GPIO 25 -> sound 2`
- `GPIO 26 -> sound 3`

Button assumptions:

- button wired from GPIO to `GND`
- internal pull-up enabled
- press = logic low
- debounce = `20 ms`

Serial logging now prints lines like:

```text
BUTTON play sound=2 pin=25 detect_us=123456 send_call_us=410 result=ok
```

This gives the local client-side software cost from stable button detection to completion of the transport send call.

Optional client marker pins:

- `GPIO 4` pulses when a press is accepted
- `GPIO 13` pulses after the send call returns

## Sound Server

The `sound_server` firmware now logs latency stages for each received packet.

Example log shape:

```text
LATENCY rx transport=ESP-NOW cmd=1 sound=2 seq=17 t_us=456789
LATENCY playback-armed transport=ESP-NOW sound=2 seq=17 from_rx_us=8200 open_us=2100 decoder_us=1800 total_prepare_us=3900
LATENCY first-copy-start transport=ESP-NOW sound=2 seq=17 from_rx_us=9100
LATENCY first-copy-end transport=ESP-NOW sound=2 seq=17 from_rx_us=12500 copy_call_us=3300 result=more
```

Meaning:

- `LATENCY rx`: packet has been accepted by the sound server
- `playback-armed`: SD open + WAV decoder setup completed
- `first-copy-start`: the firmware is just about to enter the first `wavCopier.copy()` call
- `first-copy-end`: that first `wavCopier.copy()` call has returned

Optional server marker pins:

- `GPIO 32` pulses on packet receipt
- `GPIO 33` pulses on first audio copy

## Important Measurement Note

The client and server clocks are not synchronized, so serial timestamps alone cannot directly give the full end-to-end button-to-sound latency across both boards.

They are still useful for breaking down local costs on each side.

For true end-to-end latency, use one of these methods:

1. Logic analyzer or oscilloscope on the marker GPIOs.
2. Scope on a marker GPIO and on the analog audio output.
3. Future protocol extension carrying sender timestamps and an acknowledgement path.

## Suggested Bench Wiring For Latency Work

## Trigger Client Buttons

For each button:

- one side to the chosen GPIO
- the other side to `GND`

Suggested first four buttons:

- button 0 -> `GPIO 32`
- button 1 -> `GPIO 33`
- button 2 -> `GPIO 25`
- button 3 -> `GPIO 26`

## Probe Pins

- client detect marker -> `GPIO 4`
- client send marker -> `GPIO 13`
- server receive marker -> `GPIO 32`
- server first-audio marker -> `GPIO 33`

## What To Measure First

1. Compare `UART` and `ESP-NOW` using the same sound file.
2. Repeat with a short WAV and a longer WAV.
3. Look at how much time is spent before `playback-armed`.
4. Look at the additional delay from `playback-armed` to `first-copy-start`.
5. Look at how long the first `wavCopier.copy()` call itself is taking.
6. Finally compare marker GPIOs against actual analog audio onset.

## Current Interpretation

The earlier `first-copy` marker was placed after a potentially blocking `wavCopier.copy()` call, so it could overstate audible startup delay by including a large chunk of playback time.

The revised markers split that into:

- the moment the first copy call starts
- the time spent inside that first copy call

This should make it much easier to separate:

- true audio-start latency
- SD / decoder preparation time
- time spent blocked inside the audio copy path

## Likely Next Improvements

- add support for 8 buttons instead of 4 once the first bench is stable
- add an acknowledgement packet for round-trip timing
- preload or cache the most frequently used WAV metadata if SD open time dominates
- decide whether low-latency sounds should stay as WAV on SD or move to a preloaded RAM / flash strategy
