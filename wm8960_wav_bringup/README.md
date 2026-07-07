# WM8960 WAV Bring-Up

This is a deliberately small PlatformIO project for isolating WM8960 WAV playback behaviour from the larger `sound_server` application.

The intent is to answer a narrow question:

- can this ESP32 + WM8960 + SD hardware play fixed-format `22050 Hz`, stereo, `16-bit` WAV files cleanly?

It removes the trigger transport, sound-map layer, and other sound-server concerns so we can focus on:

- WM8960 initialization
- SD card file access
- WAV decoding
- whether keeping the WM8960 initialized once is clean or noisy

## Wiring

### WM8960

- `GPIO 22` -> `SDA`
- `GPIO 21` -> `SCL`
- `GPIO 27` -> `BCLK`
- `GPIO 26` -> `LRCLK` / `WS`
- `GPIO 25` -> `DIN`

### microSD

- `GPIO 5` -> `CS`
- `GPIO 18` -> `SCK`
- `GPIO 19` -> `MISO`
- `GPIO 23` -> `MOSI`

## What It Does

At boot, the firmware:

1. initializes Serial at `115200`
2. initializes the SD card
3. initializes the WM8960 at fixed `22050 Hz`, `2 ch`, `16-bit`
4. exposes a small serial command interface

The project supports two WM8960 modes:

- `fixed`
  - keeps the codec initialized once and only updates I2S-side audio info
- `reinit`
  - allows the WAV header notification path to reinitialize the codec
- `legacy`
  - recreates the older known-good style more closely:
    plain WM8960 startup at `44100 Hz`, lower default output volume, then let the WAV header reconfigure playback

This should help answer whether the persistent noise is tied specifically to the fixed one-time initialization approach.

## Commands

- `help`
- `list`
  - list files in the SD root
- `play <file>`
  - play a WAV file from the SD root, for example `play 0-putt.wav`
- `stop`
- `tone on`
- `tone off`
- `volume <0-100>`
- `mode fixed`
- `mode reinit`
- `mode legacy`

## Build And Flash

```powershell
cd .\wm8960_wav_bringup
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run --target upload
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --echo
```

## Suggested Test Sequence

1. Boot the project and confirm SD and WM8960 initialization succeed.
2. Run `tone on` and listen for whether the synthetic tone is clean.
3. Run `list` and identify a known-good WAV file on the SD card.
4. Run `play <file>` in `mode fixed`.
5. Run `mode reinit`, then replay the same file.
6. Run `mode legacy`, then replay the same file.
6. Compare:
   - tone clean / WAV noisy
   - both noisy
   - fixed noisy but reinit clean
   - legacy clean while fixed/reinit are noisy
   - both clean

That matrix should tell us much more quickly whether the remaining issue is:

- WM8960 base configuration
- WAV decode path
- fixed one-time configuration
- or something electrical outside the firmware
