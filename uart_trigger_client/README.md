# UART Trigger Client

## Purpose

This project is the first embedded sender for the AlexsAudio wired trigger path.

It acts as a small ESP32-side client that:

- accepts the same text CLI and binary trigger packets on its USB serial port
- forwards valid trigger packets over a wired UART link
- reuses the same shared packet definition as the `sound_server`

This lets us bench-test two ESP32 boards together while keeping the browser WebSerial page as an optional host-side harness.

## Wiring

Suggested direct bench wiring between the two ESP32 boards:

- client `TX GPIO 17` (`TXD2` on the `ESP32 DEV KIT V1` board) -> sound_server `RX GPIO 16` (`RXD2`)
- client `RX GPIO 16` (`RXD2` on the `ESP32 DEV KIT V1` board) -> sound_server `TX GPIO 17` (`TXD2`) (optional for future two-way use)
- client `GND` -> sound_server `GND`

Both projects currently use:

- `115200 8N1`

Board reference:

- [ESP32 DEV KIT V1 pinout reference](../docs/ESP32-DOIT-DEV-KIT-v1-pinout-mischianti.png)

## Host Control

On the client USB serial port you can use either:

- text commands such as `play 3`, `stop`, and `ping`
- the browser WebSerial page at `tools/webserial_trigger.html`

In both cases the client forwards the same packet format over the wired UART link.

## Build And Run

From this folder:

```powershell
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe run --target upload
C:\Users\alanb\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --echo
```
