# Lab 3 — Serial Communication Smart Home System

**Module:** UFMF11-15-2 — Embedded Systems Engineering
**Assessor:** Dr. Sunny Atuba
**Student:** Mohammad Hajjeyah (24060397)

## Overview
PC-to-NUCLEO communication over UART using the existing USB connection. A smart home alarm monitoring system controlled entirely through PC keyboard inputs, with all status messages printed to the serial terminal at 115200 baud. Six commands handle alarm toggling, state querying, system reset, and a continuous monitoring mode that updates every 2 seconds.

## Commands
| Key | Action |
|---|---|
| 1 | Toggle gas alarm ON/OFF |
| 2 | Query current gas alarm state |
| 3 | Query current temperature alarm state |
| 4 | Toggle temperature alarm ON/OFF |
| 5 | Reset both alarms |
| 6 | Toggle continuous monitoring mode (2 s updates) |

## Files
- `Lab3_Logbook_Report.docx` — full lab report
- `Lab3_Code.txt` — final Mbed C++ source code
- `videos/` — serial terminal demonstration
- `photos/` — setup screenshots

## Hardware
- NUCLEO-F439ZI connected via USB to Chromebook
- No external wiring required — uses existing USB serial link

## Key Implementation Notes
- `RawSerial` used at 115200 baud (Mbed OS version does not support `UnbufferedSerial`)
- Switch-case structure for clean command dispatch inside main loop
- `Timer` object with `timer.read() >= 2.0f` polling for 2-second monitoring updates (older API required by Mbed OS version)

## Video Evidence
1. **Startup menu** — command list printed to terminal
2. **All six commands** — each key tested in sequence
3. **Continuous monitoring** — 2-second periodic updates demonstrated
