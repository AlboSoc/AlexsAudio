# Sound Server

## Purpose

This project is the first step toward the dedicated sound-server ESP32.

Its immediate goals are:

- initialize the WM8960 audio output hardware
- initialize the SD card hardware
- scan the SD card for sound files named like `0-bird.wav`
- build a simple `sound_id -> file path` table
- provide simple serial commands for bring-up and debugging

This lets us prove that the core hardware pieces can coexist on one ESP32 before adding:

- WAV playback from SD card
- `ESP-NOW`
- `UART` / `RS-485`
- the final game-trigger protocol

## Current Hardware Assumptions

### WM8960

- `I2C SDA -> GPIO 22`
- `I2C SCL -> GPIO 21`
- `I2S BCLK -> GPIO 27`
- `I2S WS   -> GPIO 26`
- `I2S DATA -> GPIO 25`

### microSD over SPI

- `CS   -> GPIO 5`
- `SCK  -> GPIO 18`
- `MISO -> GPIO 19`
- `MOSI -> GPIO 23`

## Current Behavior

At startup the project:

1. prints the pin configuration
2. initializes the SD card
3. scans the root directory for files whose names begin with `<id>-`
4. initializes the WM8960
5. starts a simple serial command interface

It also includes a tone generator for verifying that the audio path is alive independently of SD-file playback.

## Expected Sound File Naming

Examples:

- `0-bird.wav`
- `1-dog.wav`
- `2-golf-swing.wav`

Only the leading integer before the first `-` is used as the `sound_id`.

## Serial Commands

- `help`
  - print available commands
- `list`
  - print the discovered sound map
- `tone on`
  - enable a test tone through the WM8960
- `tone off`
  - disable the test tone
- `play <id>`
  - currently resolves and prints the matching file path for that ID

The `play` command is currently a placeholder for the next stage, where we hook SD-based WAV playback into the audio output path.

## Build And Run

From this folder:

```powershell
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run --target upload
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --echo
```

## Example session output

Here's some example output from running this:

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
  0 -> 0-ding.wav
  1 -> 1-Windows-Background.wav
  2 -> 2-Windows-Notify-Email.wav
  3 -> 3-Windows-Unlock.wav
  4 -> 4-BasicDoneListeningEarcon.wav
  5 -> 5-BasicListeningEarcon.wav
  6 -> <none>
  7 -> <none>
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
  play <id> - resolve and print the file mapped to sound ID
list
Discovered sound map:
  0 -> 0-ding.wav
  1 -> 1-Windows-Background.wav
  2 -> 2-Windows-Notify-Email.wav
  3 -> 3-Windows-Unlock.wav
  4 -> 4-BasicDoneListeningEarcon.wav
  5 -> 5-BasicListeningEarcon.wav
  6 -> <none>
  7 -> <none>
tone on
Tone enabled.
tone off
Tone disabled.
play 1
Resolved sound ID 1 -> 1-Windows-Background.wav
Playback from SD is the next implementation step.
help
Commands:
  help      - show this help
  list      - show discovered sound IDs
  tone on   - enable WM8960 test tone
  tone off  - disable WM8960 test tone
  play <id> - resolve and print the file mapped to sound ID
```

## Next Step

Once this project is behaving cleanly, the next useful upgrade is:

- replace the placeholder `play <id>` behavior with real WAV playback from SD

