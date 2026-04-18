# Lab 1 — Creating a Blinky LED

**Module:** UFMF11-15-2 — Embedded Systems Engineering
**Assessor:** Dr. Sunny Atuba
**Student:** Mohammad Hajjeyah (24060397)

## Overview
First introduction to Mbed, Keil Studio Cloud, and the NUCLEO-F439ZI board. Five tasks programming the three onboard LEDs (LD1, LD2, LD3) with different timing and sequencing patterns using `DigitalOut` and `thread_sleep_for()`.

## Tasks
1. **Task i** — All three LEDs blink simultaneously at 500 ms on/off
2. **Task ii** — LEDs blink sequentially (LED1 → LED2 → LED3), 500 ms each
3. **Task iii** — All three LEDs blink simultaneously at 300 ms on/off
4. **Task iv** — Repeating pattern: LED1 for 200 ms, LED2 for 400 ms, LED3 for 600 ms
5. **Task v (Main Task)** — All three LEDs blink together 5 times at 200 ms, then LED1 holds on while LED2 and LED3 stay off

## Files
- `Lab1_Logbook_Slides.pdf` — full lab logbook (slide format)
- `Lab1_Code.txt` — final Mbed C++ source code (Task v)
- `videos/` — demonstration recordings for each task
- `photos/` — hardware and build evidence

## Hardware
- NUCLEO-F439ZI development board connected via USB to Chromebook
- No external components — onboard LEDs only

## Video Evidence
1. **Task i** — simultaneous 500 ms blink
2. **Task ii** — sequential blink
3. **Task iii** — simultaneous 300 ms blink
4. **Task iv** — 200 / 400 / 600 ms pattern
5. **Task v** — 5 blinks then LED1 holds

## Build
Project compiled in Keil Studio Cloud. `.bin` file downloaded and dragged onto the NUCLEO-F439ZI mass storage drive (NODE_F439ZI) to flash.
