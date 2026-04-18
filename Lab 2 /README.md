# Lab 2 — Secure Access Control System

**Module:** UFMF11-15-2 — Embedded Systems Engineering
**Assessor:** Dr. Sunny Atuba
**Student:** Mohammad Hajjeyah (24060397)

## Overview
Secure 4-digit passcode entry system on the NUCLEO-F439ZI using six tactile switches as digit inputs (0–5) and three onboard LEDs as status indicators. The system implements a finite state machine with three states: NORMAL, WARNING, and LOCKDOWN. After three incorrect code attempts the system enters a warning phase, and further failures trigger a full lockdown that can only be cleared with the admin code.

## Features
- 4-digit user passcode and 4-digit admin passcode
- State machine: NORMAL → WARNING → LOCKDOWN
- LED feedback for current system state
- Event log counter LED blinks once per historical lockdown
- Debounced button reads using `DigitalIn` with `PullDown` mode

## Files
- `Lab2_Logbook_Report.docx` — full lab report
- `Lab2_Code.txt` — final Mbed C++ source code
- `videos/` — demonstration recordings
- `photos/` — hardware setup images

## Hardware Setup
| Component | NUCLEO Pin | Notes |
|---|---|---|
| Button 0 (digit 0) | D2 | PullDown mode, other side to 3.3 V |
| Button 1 (digit 1) | D3 | PullDown mode |
| Button 2 (digit 2) | D4 | PullDown mode |
| Button 3 (digit 3) | D5 | PullDown mode |
| Button 4 (digit 4) | D6 | PullDown mode |
| Button 5 (digit 5) | D7 | PullDown mode |
| Warning LED | LED1 | Onboard |
| Blocked LED | LED2 | Onboard |
| Event log LED | LED3 | Onboard |

## Video Evidence
1. **Correct code entry** — normal state maintained
2. **Incorrect attempts** — warning state triggered after 3 failures
3. **Lockdown and admin unlock** — full lockdown cleared via admin code
4. **Event log counter** — blink pattern reflecting lockdown history
