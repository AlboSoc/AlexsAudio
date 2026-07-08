# Sound Server

## Purpose

This project is the first step toward the dedicated sound-server ESP32.

Its immediate goals are:

- initialize the WM8960 audio output hardware
- initialize the SD card hardware
- scan the SD card for sound files named like `0-putt.wav`
- build a simple `sound_id -> file path` table
- provide simple serial commands for bring-up and debugging

This lets us prove that the core hardware pieces can coexist on one ESP32 before adding:

- WAV playback from SD card
- `UART` / `RS-485`
- the final game-trigger protocol

## Current Hardware Assumptions

### WM8960

- `I2C SDA -> GPIO 22`
- `I2C SCL -> GPIO 21`
- `I2S BCLK -> GPIO 27`
- `I2S WS   -> GPIO 26`
- `I2S DATA -> GPIO 25`

Practical wiring note:

- on the breadboard setup, adding explicit `3.3 V` pull-ups on `SDA` and `SCL` materially improved reliability
- the WM8960 breakout already has pull-ups, but the bench wiring behaved better with stronger external pull-ups as well
- if this setup is rebuilt, include the pull-ups from the start rather than relying on long Dupont wiring alone

### microSD over SPI

- `CS   -> GPIO 5`
- `SCK  -> GPIO 18`
- `MISO -> GPIO 19`
- `MOSI -> GPIO 23`
- current runtime SPI clock: `10 MHz`

### Trigger UART

- `RX   -> GPIO 16` (`RXD2` on the `ESP32 DEV KIT V1` board)
- `TX   -> GPIO 17` (`TXD2` on the `ESP32 DEV KIT V1` board)
- `115200 8N1`
- currently disabled by default during ESP-NOW latency bring-up to avoid noise from a floating RX pin

### ESP-NOW

- current channel: `1`
- receiver runs in `WIFI_STA` mode
- current sender behavior is simple broadcast receive for bench bring-up

Board reference:

- [ESP32 DEV KIT V1 pinout reference](../docs/ESP32-DOIT-DEV-KIT-v1-pinout-mischianti.png)

## Current Behavior

At startup the project:

1. prints the pin configuration
2. initializes the SD card
3. scans the root directory for files whose names begin with `<id>-`
4. initializes the WM8960
5. starts a simple serial command interface

It also includes a tone generator for verifying that the audio path is alive independently of SD-file playback.

In addition to the text CLI, the firmware now accepts small binary trigger packets on the same USB serial port.
The same packet path is active over ESP-NOW, and can also be enabled on the dedicated wired UART when needed.

## Expected Sound File Naming

Examples:

- `0-putt.wav`
- `1-applause.wav`
- `2-ball-going-in-hole.wav`

Only the leading integer before the first `-` is used as the `sound_id`.

## Serial Commands

- `help`
  - print available commands
- `list`
  - print the discovered sound map and parsed WAV format details
- `tone on`
  - enable a test tone through the WM8960
- `tone off`
  - disable the test tone
- `play <id>`
  - plays the matching WAV file for that ID through the WM8960 output path
- `volume <0-100>`
  - sets the WM8960 output volume percentage live without reflashing

## UART Trigger Packet

The sound server now accepts a fixed-size trigger packet on:

- the USB serial port used for the text CLI and WebSerial bring-up
- the dedicated wired trigger UART on `GPIO 16/17` when `ENABLE_TRIGGER_UART` is enabled
- ESP-NOW on channel `1`

Packet layout:

```text
byte 0: magic    = 0xA5
byte 1: version  = 0x01
byte 2: command  = 0x01 play, 0x02 stop, 0x03 ping
byte 3: soundId  = 0..255
byte 4: sequence = sender-chosen rolling counter
byte 5: checksum = xor of bytes 0..4
```

Notes:

- valid `play` packets are converted into the same internal `PlaySoundCommand` used by `play <id>`
- `stop` stops the current playback immediately
- `ping` produces a simple serial log response for transport bring-up
- normal text commands and binary packets can share the same serial port, but not at the same instant from two different host applications
- the same packet format is also consumed on the dedicated wired trigger UART
- the same packet format is also consumed over ESP-NOW

## Currently Known-Good WAV Format

The current working baseline is:

- PCM WAV (`fmt=1`)
- `44100 Hz`
- `stereo`
- `16-bit`
- minimal header with an easy-to-find `data` chunk

The generated files in `sound_server/audio_44100` are now the primary reference set.

The firmware is now configured to stay at `44100 Hz` throughout startup and playback, which avoids the earlier problematic `22050 Hz` runtime path.

## Latency Notes

Recent latency work showed that:

- trigger send time on the client is tiny
- ESP-NOW transfer time is tiny
- the dominant delay is inside the server audio-start path

Two important details came out of that work:

- the earlier `first-copy` marker was too late, because it sat after a potentially blocking `wavCopier.copy()` call
- the server now logs both `first-copy-start` and `first-copy-end` so we can tell whether the delay is before the first copy or inside it

The server also now uses a faster SD SPI clock and has the wired UART receiver disabled by default during ESP-NOW latency testing.

The dedicated latency serial logs and marker pins are now disabled by default in normal builds.
If latency investigation is needed again, re-enable them in `src/sound_server_config.h`.

## Building The Runtime Sound Pack

The intended source assets now live in `sound_server/audio_originals` as `.ogg` files.

There is a helper script at `tools/convert_audio_originals.ps1` to batch-convert those source assets into the runtime WAV files used by the firmware.

From the `AlexsAudio` repo root:

```powershell
.\tools\convert_audio_originals.ps1
```

By default it reads from `.\sound_server\audio_originals` and writes the generated WAV files to `.\sound_server\audio`.

This keeps the compressed source assets and the firmware-ready WAV outputs separate and reproducible.

For the current setup, the preferred runtime pack lives in `.\sound_server\audio_44100`.

The current preferred command is:

```powershell
.\tools\convert_audio_originals.ps1 -SampleRate 44100
```

It uses `ffmpeg` to:

- strip metadata
- force `stereo`
- force the requested sample rate
- force `16-bit PCM`

## Build And Run

From this folder:

```powershell
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run --target upload
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --echo
```

## WebSerial Test Page

A single-page WebSerial test app lives at `tools/webserial_trigger.html`.

Because WebSerial requires a secure context, serve the repo over localhost before opening the page. From the `AlexsAudio` repo root, one simple option is:

```powershell
py -m http.server 8000
```

Then open:

```text
http://localhost:8000/tools/webserial_trigger.html
```

Suggested test flow:

1. Close any existing serial monitor so the browser can open the port.
2. Click `Connect Serial Port`.
3. Choose the ESP32 USB serial device.
4. Click one of the `Play` buttons, or send `Ping` / `Stop`.
5. Watch the log pane for `PACKET ...` responses from the firmware.

The page currently speaks the same packet format described above and is meant as a bench bring-up tool for Phase 1 of the trigger transport work.

## Example session output

Here's some example output from an earlier debug-heavy run:

```
PS C:\Alan\code\audio\AlexsAudio\sound_server> C:\Users\alanb\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --echo
--- Terminal on COM5 | 115200 8-N-1
--- Available filters and text transformations: debug, default, direct, esp32_exception_decoder, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at https://bit.ly/pio-monitor-filters
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
ets Jun  8 2016 00:22:57

rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:1184
load:0x40078000,len:13232
load:0x40080400,len:3028
entry 0x400805e4

=== AlexsAudio Sound Server Bring-Up ===
WM8960 I2C SDA : GPIO 22
WM8960 I2C SCL : GPIO 21
WM8960 I2S BCLK: GPIO 27
WM8960 I2S WS  : GPIO 26
WM8960 I2S DATA: GPIO 25
SD CS          : GPIO 5
SD SCK         : GPIO 18
SD MISO        : GPIO 19
SD MOSI        : GPIO 23
SD initialization succeeded.
Discovered sound map:
  0 -> 0-putt.wav
  1 -> 1-applause.wav
  2 -> 2-ball-going-in-hole.wav
  3 -> 3-ball-landing-in-water.wav
  4 -> 4-drive.wav
  5 -> 5-ball-landing-on-sand.wav
  6 -> 6-ball-landing-on-grass.wav
  7 -> 7-sad-groans.wav
mtb_wm8960_set_wire
[W] WM8960Stream.h : 190 - Setup features: 24
mtb_wm8960_init
mtb_wm8960_write 0xf = 0x0
mtb_wm8960_write 0x19 = 0xc0
mtb_wm8960_write 0x1a = 0x1f8
mtb_wm8960_write 0x31 = 0xf7
mtb_wm8960_write 0x2f = 0xc
mtb_wm8960_write 0x22 = 0x100
mtb_wm8960_write 0x25 = 0x100
mtb_wm8960_write 0x2 = 0x179
mtb_wm8960_write 0x3 = 0x179
mtb_wm8960_write 0x5 = 0x0
mtb_wm8960_write 0x22 = 0x100
mtb_wm8960_write 0x25 = 0x100
mtb_wm8960_write 0x28 = 0x179
mtb_wm8960_write 0x29 = 0x179
mtb_wm8960_write 0x5 = 0x0
mtb_wm8960_adjust_input_volume
_mtb_wm8960_adjust_volume
mtb_wm8960_write 0x0 = 0x92
mtb_wm8960_write 0x1 = 0x192
mtb_wm8960_adjust_speaker_output_volume
_mtb_wm8960_adjust_volume
mtb_wm8960_write 0x28 = 0x58
mtb_wm8960_write 0x29 = 0x158
mtb_wm8960_adjust_heaphone_output_volume
_mtb_wm8960_adjust_volume
mtb_wm8960_write 0x2 = 0x58
mtb_wm8960_write 0x3 = 0x158
mtb_wm8960_activate
mtb_wm8960_write 0x19 = 0xc0
mtb_wm8960_write 0x1a = 0x1f8
mtb_wm8960_write 0x2f = 0xc
mtb_wm8960_configure_clocking
_mtb_wm8960_setup_pll
mtb_wm8960_write 0x34 = 0x38
mtb_wm8960_write 0x35 = 0x0
mtb_wm8960_write 0x36 = 0x0
mtb_wm8960_write 0x37 = 0x0
mtb_wm8960_write 0x1a = 0x1f9
mtb_wm8960_write 0x4 = 0x5
mtb_wm8960_write 0x7 = 0x2
mtb_wm8960_adjust_speaker_output_volume
_mtb_wm8960_adjust_volume
mtb_wm8960_write 0x28 = 0x4e
mtb_wm8960_write 0x29 = 0x14e
mtb_wm8960_adjust_heaphone_output_volume
_mtb_wm8960_adjust_volume
mtb_wm8960_write 0x2 = 0x4e
mtb_wm8960_write 0x3 = 0x14e
WM8960 initialization succeeded.
Commands:
  help      - show this help
  list      - show discovered sound IDs
  tone on   - enable WM8960 test tone
  tone off  - disable WM8960 test tone
  play <id> - play the WAV file mapped to sound ID
list
Discovered sound map:
  0 -> 0-putt.wav
  1 -> 1-applause.wav
  2 -> 2-ball-going-in-hole.wav
  3 -> 3-ball-landing-in-water.wav
  4 -> 4-drive.wav
  5 -> 5-ball-landing-on-sand.wav
  6 -> 6-ball-landing-on-grass.wav
  7 -> 7-sad-groans.wav
tone on
Tone enabled.
tone off
Tone disabled.
play 1
Playing sound ID 1 -> 1-applause.wav
Playback finished for sound ID 1
help
Commands:
  help      - show this help
  list      - show discovered sound IDs
  tone on   - enable WM8960 test tone
  tone off  - disable WM8960 test tone
  play <id> - play the WAV file mapped to sound ID
```

## Next Steps

The next useful upgrades for this project are:

- integrate the working trigger-and-playback path into a small game-facing demo flow
- decide whether the sound server should emit any reply or acknowledgement traffic on either transport
- decide whether to keep the current broadcast ESP-NOW bring-up mode or tighten it to explicit peer MACs
