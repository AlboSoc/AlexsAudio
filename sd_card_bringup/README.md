# SD Card Bring-Up

## Purpose

This folder is for bringing up the microSD card hardware separately before mixing SD-card access into the larger two-ESP32 audio-server project.

The goal of this stage is to prove:

- the physical breakout board is soldered and wired correctly
- the ESP32 can initialize the card reliably
- files can be listed and read successfully
- the SD setup is stable before we depend on it for audio playback

This should reduce confusion later by separating SD-card issues from:

- audio codec / amplifier issues
- transport issues (`ESP-NOW` or `UART` / `RS-485`)
- file-format / playback logic

## Hardware

Current breakout board:

- Adafruit MicroSD Card Breakout Board

Reference pages:

- Adafruit guide: https://learn.adafruit.com/adafruit-microsd-spi-sdio
- Pi Hut product page: https://thepihut.com/products/adafruit-microsd-card-breakout-board

## Important Notes

This breakout supports `SPI` and `SDIO`, but the recommended first bring-up path is `SPI` because it is simpler.

Also note:

- the board is intended for `3.3 V` logic
- there is no onboard level shifting
- there is no onboard `5 V` regulator for the card interface

That is fine for an ESP32, but it means correct wiring and power are important.

## Recommended Bring-Up Approach

Start with the smallest useful test:

1. Wire the breakout to the ESP32 using `SPI`
2. Initialize the card
3. Print card information to the serial monitor
4. List files in the root directory
5. Read a small known text file
6. Only after that, move on to audio-file lookup and playback work

## Suggested First Software Test

The first SD-card test project should do only these things:

- boot and print to serial
- initialize the SD card over `SPI`
- print whether initialization succeeded
- list the root directory contents
- read a small test file such as `hello.txt`
- optionally measure a simple read speed

Keep this test independent of:

- WM8960 audio code
- SD-based audio playback code
- `ESP-NOW`
- `UART` / `RS-485`

## Wiring Notes

This first test project assumes the common ESP32 `VSPI` pinout:

- `CS   -> GPIO 5`
- `SCK  -> GPIO 18`
- `MISO -> GPIO 19`
- `MOSI -> GPIO 23`
- `VCC  -> 3V3`
- `GND  -> GND`

This is intentionally separate from the WM8960 control bus, which is now using:

- `SDA -> GPIO 22`
- `SCL -> GPIO 21`

So the SD-card and WM8960 control wiring should no longer fight over `GPIO 19`.

## SD Card Preparation

Suggested first test contents on the card:

- `hello.txt`
- one or two small dummy files for directory listing tests

Later, for the sound-server project, the card will likely hold files such as:

- `0-bird.wav`
- `1-dog.wav`
- `2-golf-swing.wav`

## Bring-Up Checklist

- Header soldered to breakout board
- Breakout mounted securely
- `3.3 V` and `GND` wired correctly
- SPI lines wired correctly
- SD card inserted
- Card formatted appropriately
- ESP32 can initialize card
- Root directory listing works
- A known file can be read successfully

## Next Step After Hardware Bring-Up

Once the wiring is complete and stable, the next sensible step is to create a tiny dedicated SD-card test project in `AlexsAudio` and validate basic file access before integrating SD support into the sound-server code.

That tiny dedicated test project now lives in this folder.

## Project Files

- `platformio.ini`
- `src/main.cpp`

The sketch currently:

- prints the chosen SPI pins
- initializes the SD card over SPI
- prints card type and size
- lists the root directory
- tries to read `/hello.txt`

## How To Run

From this folder:

```powershell
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run --target upload
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe device monitor -b 115200
```

## Expected Test Card Contents

Put a simple file called `hello.txt` in the root of the SD card, for example:

```text
Hello from the SD card.
```

## Example Successful Output

Here is a real example of successful serial output from this bring-up test:

```text
ets Jun  8 2016 00:22:57

rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:1184
load:0x40078000,len:13232
load:0x40080400,len:3028
entry 0x400805e4

=== SD Card Bring-Up ===
CS   : GPIO 5
SCK  : GPIO 18
MISO : GPIO 19
MOSI : GPIO 23
SPI frequency: 1000000 Hz
SD initialization succeeded.
Card type     : SDHC/SDXC
Card size     : 3796 MB
Filesystem total: 3786 MB
Filesystem used : 0 MB
Listing directory: /
  [DIR ] System Volume Information
Listing directory: /System Volume Information
  [FILE] WPSettings.dat (12 bytes)
  [FILE] IndexerVolumeGuid (76 bytes)
  [FILE] hello.txt (23 bytes)
Reading /hello.txt
  File contents:
Hello from the SD card!
```
