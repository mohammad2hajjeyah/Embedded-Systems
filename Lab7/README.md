# Lab 7 — DC Motor Driving using Relays and Interrupts

**Module:** UFMF11-15-2 Embedded Systems
**Institution:** UWE Bristol — School of Engineering
**Supervisor:** Dr. Sunny Atuba
**Student:** Mohammad Hajjeyah (24060397)
**Board:** NUCLEO-F439ZI
**IDE:** Keil Studio Cloud (Mbed OS)

---

## Overview

This lab integrates a DC motor, a 2-channel relay module, a PIR motion sensor and two direction-indicator LEDs into the smart home system built up over Labs 3–6. The lab introduces **hardware interrupts** as an alternative to polling — direction buttons, limit switches and the PIR sensor all use `InterruptIn` callbacks rather than being checked every loop iteration.

The system carries forward the full Lab 6 stack (I2C LCD, matrix keypad, LM35 temperature sensor, MQ-2 gas sensor, active buzzer) and adds:

- Bidirectional motor control through two relay channels (tri-state control pattern from Example 7.1)
- Gate finite-state machine with four states: CLOSED, OPENING, OPEN, CLOSING
- PIR-based intruder detection with a new alarm source
- Green/red LEDs that mirror the motor direction
- Serial commands `m` and `g` for motor and gate status reporting

---

## Hardware

| Component | NUCLEO Pin | Notes |
|---|---|---|
| 2-channel relay — IN1 | PF_2 | Active-low, driven via `DigitalInOut` + OpenDrain |
| 2-channel relay — IN2 | PE_3 | Same pattern as IN1 |
| Relay VCC / GND | 5V / GND | Powered from MB-102 rail |
| DC motor lead A | Relay COM1 | Motor through NO1/NC1 + NO2/NC2 for direction reversal |
| DC motor lead B | Relay COM2 | 5V on both NO terminals, GND on both NC terminals |
| Dir1 button (Open) | PF_9 | `InterruptIn`, PullUp, falling edge |
| Dir2 button (Close) | PF_7 | `InterruptIn`, PullUp, falling edge |
| Dir1LS (limit switch) | PG_3 | `InterruptIn`, PullUp, falling edge |
| Dir2LS (limit switch) | PF_8 | `InterruptIn`, PullUp, falling edge |
| PIR sensor OUT | PG_0 | `InterruptIn`, rising edge, 5V supply |
| Green LED (Opening) | D8 | Via 330Ω to GND |
| Red LED (Closing) | D9 | Via 330Ω to GND |
| 20x4 LCD SDA / SCL | PB_9 / PB_8 | Carried from Lab 6 |
| 4x4 Keypad — rows | PB_3, PB_5, PC_7, PA_15 | Carried from Lab 5 |
| 4x4 Keypad — columns | PB_12, PB_13, PB_15, PC_6 | Carried from Lab 5 |
| LM35 Temperature | A1 | Carried from Lab 4 |
| Gas Sensor (digital) | PD_2 | Carried from Lab 3 |
| Active Buzzer | PE_10 | Carried from Lab 3 |

---

## Software architecture

The code is a single-file `main.cpp` organised into the following modules:

- **LCD driver** — bit-banged PCF8574 I2C driver in 4-bit mode (carried from Lab 6)
- **Motor control** — finite-state machine mirroring Example 7.1, updated every 100 ms
- **Gate control** — wrapper layer mirroring Example 7.2, manages gate status transitions
- **Interrupt service routines** — kept short, only set flags or update small state
- **Keypad scanner** — 3-state debounced scanner returning on release (carried from Lab 5/6)
- **Serial command handler** — implements `m` (motor status), `g` (gate status), `h` (help)
- **Main loop** — 10 ms tick driving five staggered counters for motor, LCD, sensors, report timeout, and periodic alarm broadcast

All shared state accessed by both ISRs and the main loop is marked `volatile`.

---

## Key features

- **Interrupt-driven inputs** — direction buttons, limit switches and PIR all use external interrupts (no polling overhead)
- **Non-blocking main loop** — 10 ms tick with integer counters for scheduling sub-systems at different rates
- **Screen-state LCD** — six screen layouts (prompt, gas, temperature, alarm, gate, intruder) driven by a `ScreenState` enum
- **Tri-state motor control** — relay pins toggle between `OpenDrain input()` (relay de-energised) and `output() = 0` (relay energised) to ensure only one relay is ever active at a time
- **5-digit alarm deactivation code** — `13709` clears intruder, gas, and temperature alarms simultaneously
- **Keypad gate control** — keys `A` and `B` provide manual open/close in addition to the hardware buttons, satisfying the main task's "matrix keypad to provide user control" requirement

---

## Testing

| Test scenario | Expected behaviour |
|---|---|
| System boot | LCD shows prompt, serial prints banner, buzzer silent, LEDs off |
| Press Dir1 | Motor spins direction 1, green LED on, LCD shows "GATE OPENING" |
| Press Dir1LS | Motor stops, LED off, LCD shows "GATE OPEN" |
| Press Dir2 | Motor spins direction 2, red LED on, LCD shows "GATE CLOSING" |
| Press Dir2LS | Motor stops, LED off, LCD shows "GATE CLOSED" |
| Wave hand at PIR | LCD flashes "INTRUDER ALERT", buzzer sounds, serial logs event |
| Enter `13709` | All alarms clear, buzzer silences, LCD returns to prompt |
| Press key `4` | LCD briefly shows gas detector report |
| Press key `5` | LCD briefly shows temperature report |
| Press key `A` | Gate opens (same as Dir1 button) |
| Press key `B` | Gate closes (same as Dir2 button) |
| Serial `m` | Prints current motor direction |
| Serial `g` | Prints current gate status |

All scenarios were verified on the physical rig and recorded for submission.

---

## Compatibility notes

The board environment uses an older Mbed OS version, so the code avoids:

- `UnbufferedSerial` — uses `RawSerial` instead
- Chrono literals (`2ms`, `100us`) — uses `wait_ms()` / `wait_us()` instead
- `ThisThread::sleep_for(Xms)` syntax

The motor control uses `DigitalInOut` with `mode(OpenDrain)` and alternates between `.input()` (high-impedance, relay OFF) and `.output()` with `= 0` (pin pulled to GND, relay ON), matching the tri-state pattern from Example 7.1 of the reference text.

---

## Repository structure

---

## References

- Arm Mbed. *InterruptIn — API reference*. https://os.mbed.com/docs/mbed-os/v5.15/apis/interruptin.html
- Arm Mbed. *DigitalInOut — API reference*. https://os.mbed.com/docs/mbed-os/v5.15/apis/digitalinout.html
- armBookCodeExamples. *example_7-1* and *example_7-2* — Chapter 7 reference text, accompanying GitHub projects

---

*Mohammad Hajjeyah | 24060397 | UFMF11-15-2 | UWE Bristol*
