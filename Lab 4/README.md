# Lab 4 — Sensor Monitoring System

**Module:** UFMF11-15-2 — Embedded Systems Engineering
**Assessor:** Dr. Sunny Atuba
**Student:** Mohammad Hajjeyah (24060397)

## Overview
Analogue sensor monitoring system on the NUCLEO-F439ZI that continuously reads an LM35 temperature sensor, an MQ-2 gas sensor, and a potentiometer, evaluates warning conditions against a dynamically adjustable threshold, and activates an audible alarm via an active buzzer. All status is reported live over USB serial at 9600 baud.

## Features
- LM35 temperature reading with correct 5 V supply / 3.3 V ADC reference scaling
- MQ-2 gas detection via digital output (DOUT)
- Potentiometer-controlled threshold, mapped linearly from 20 °C to 70 °C
- Active-low buzzer drive on PE_10 (per Figure 1.9 of Lecture 4)
- Four distinct system states reported on serial: Normal, Temperature, Gas, Both

## Files
- `Lab4_Logbook_Report.docx` — full lab report
- `Lab4_Code.txt` — final Mbed C++ source code
- `videos/` — test scenario recordings (including separate buzzer demonstration)
- `photos/` — hardware setup images

## Hardware Setup
| Component | NUCLEO Pin | Notes |
|---|---|---|
| LM35 Temperature Sensor | A1 | 10 mV/°C, 5 V supply |
| Potentiometer | A0 | Wiper to A0, 3.3 V and GND on outer pins |
| MQ-2 Gas Sensor (4-pin) | D8 (PE_12) | DOUT through 3× 100 Ω divider per Figure 1.8, 5 V powered |
| Active Buzzer | PE_10 | + pin to 5 V rail, − pin to PE_10 (active-low) |

## Key Implementation Notes
- LM35 scale factor: `Temperature (°C) = ADC_float × 165.0`
- MQ-2 DOUT logic: LOW = gas detected, HIGH = clean air; pin configured with `PullUp`
- Buzzer uses active-low drive so a 3.3 V GPIO can switch a 5 V buzzer (Lecture 4, Figure 1.9)
- `RawSerial` at 9600 baud (required by this Mbed OS version)
- 1-second polling loop via `wait_us()`

## Video Evidence
1. **System Normal** — threshold set to ~30 °C, no warnings active
2. **Temperature Warning** — LM35 heated by hand, buzzer triggered
3. **Gas Warning** — MQ-2 exposed to unlit lighter gas, buzzer triggered
4. **Both Warnings** — simultaneous temperature + gas trigger, threshold set to ~40 °C in second video to demonstrate dynamic adjustment
5. **Buzzer Demonstration** — standalone test confirming audible buzzer response to each warning state
