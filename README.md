# AlexsAudio

This repository is the shared home for the AlexsAudio ESP32 audio bring-up work.

It currently contains two PlatformIO projects plus one architecture note:

- `sd_card_bringup/`
  - standalone microSD hardware and filesystem bring-up
- `sound_server/`
  - WM8960 + SD integration bring-up for the dedicated sound-server ESP32
- `TWO_ESP32_AUDIO_ARCHITECTURE.md`
  - higher-level notes about the overall system direction
- [COMMUNICATION_TRIGGER_PLAN.md](COMMUNICATION_TRIGGER_PLAN.md)
  - phased plan for transport-neutral trigger handling over CLI, `UART`, and `ESP-NOW`

## Repository Layout

```text
AlexsAudio/
|-- sd_card_bringup/
|-- sound_server/
|-- COMMUNICATION_TRIGGER_PLAN.md
|-- TWO_ESP32_AUDIO_ARCHITECTURE.md
|-- .gitignore
`-- README.md
```

## Current Focus

The current work is centered on proving the core hardware and software pieces in small steps:

1. verify reliable SD-card access
2. verify WM8960 codec bring-up
3. scan and resolve sound files from SD
4. add real WAV playback
5. later integrate transport and game-trigger behavior

## Working In This Repo

Each subproject is a separate PlatformIO project. Build, upload, and monitor from the corresponding folder.

Examples:

```powershell
cd .\sd_card_bringup
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run

cd ..\sound_server
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --echo
```

## Next Steps

- refactor local triggering around a shared `PlaySoundCommand`
- add `UART` as the first external trigger transport
- later add `ESP-NOW` as an alternative transport
