# AlexsAudio

This repository is the shared home for the AlexsAudio ESP32 audio bring-up work.

It currently contains three PlatformIO projects plus the shared design notes:

- `sd_card_bringup/`
  - standalone microSD hardware and filesystem bring-up
- `sound_server/`
  - WM8960 + SD integration bring-up for the dedicated sound-server ESP32
- `uart_trigger_client/`
  - embedded trigger sender that can drive the sound server over ESP-NOW or wired UART
- `TWO_ESP32_AUDIO_ARCHITECTURE.md`
  - higher-level notes about the overall system direction
- [COMMUNICATION_TRIGGER_PLAN.md](COMMUNICATION_TRIGGER_PLAN.md)
  - phased plan for transport-neutral trigger handling over CLI, `UART`, and `ESP-NOW`

## Repository Layout

```text
AlexsAudio/
|-- sd_card_bringup/
|-- sound_server/
|-- uart_trigger_client/
|-- shared/
|-- COMMUNICATION_TRIGGER_PLAN.md
|-- TWO_ESP32_AUDIO_ARCHITECTURE.md
|-- .gitignore
`-- README.md
```

## Current Focus

The current work is centered on building out the sound-server in stable vertical slices:

1. verify reliable SD-card access
2. verify WM8960 codec bring-up
3. scan and resolve sound files from SD
4. add real WAV playback
5. trigger playback over a shared packet-based transport
6. drive the sound server from a second ESP32 client over wired UART or ESP-NOW
7. harden transport behavior for the intended game integration

## Working In This Repo

Each subproject is a separate PlatformIO project. Build, upload, and monitor from the corresponding folder.

The ESP32 boards currently being used are labeled `ESP32 DEV KIT V1`.
For pin-label translation between GPIO numbers and the board silk screen, see:

- [ESP32 DEV KIT V1 pinout reference](docs/ESP32-DOIT-DEV-KIT-v1-pinout-mischianti.png)

Examples:

```powershell
cd .\sd_card_bringup
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run

cd ..\sound_server
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --echo

cd ..\uart_trigger_client
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --echo
```

## Next Steps

- verify two-ESP32 ESP-NOW bring-up end to end in the normal demo setup
- decide whether wired UART remains only a bench fallback or stays as a supported field option
- add any needed acknowledgement, retry, or duplicate-suppression behavior once transport testing shows the real needs
