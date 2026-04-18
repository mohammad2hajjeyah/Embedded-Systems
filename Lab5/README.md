# Lab 5 — Matrix Keypad Smart Home System

**Module:** Embedded Systems | UFMF11-15-2 | Dr. Sunny Atuba
**Author:** Mohammad Hajjeyah (24060397)
**Board:** NUCLEO-F439ZI
**IDE:** Keil Studio Cloud (Mbed OS)

---

## Overview

This lab extends the Lab 4 smart home system by adding a **4x4 matrix keypad** as the primary user input. The system now allows:

- Entry of a **4-digit deactivation code** to silence the alarm
- **Live threshold adjustment** via the potentiometer (temperature and gas)
- An **event log** of the last 5 alarm events, viewable by pressing `#`
- Continuous status reporting over USB serial at 9600 baud

The keypad wiring and scanning approach follow Figure 1.5 of Lecture 5. The Lab 4 sensor and buzzer setup is preserved; the only hardware changes are the keypad and the MB102 breadboard power module.

---

## Hardware Setup

| Component | NUCLEO Pin | Notes |
|---|---|---|
| Matrix Keypad — Rows | PB_3, PB_5, PC_7, PA_15 | DigitalOut, driven LOW one row at a time |
| Matrix Keypad — Columns | PB_12, PB_13, PB_15, PC_6 | DigitalIn with internal PullUp |
| LM35 Temperature Sensor | A1 | 5 V supply via MB102, 10 mV/°C output |
| Potentiometer | A0 | Sets both temperature and gas thresholds |
| MQ-2 Gas Sensor | D8 (PE_12) | DOUT with PullUp, 5 V supply via MB102 |
| Active Buzzer | PE_10 | Active-low drive, 5 V via MB102 |
| MB102 Power Module | — | Supplies 5 V to breadboard rails, shared GND with NUCLEO |

**Power arrangement:**
- NUCLEO powered from Chromebook USB (for flashing and serial)
- MB102 powered independently via USB adapter
- Single ground wire ties MB102 `−` rail to NUCLEO GND for a common reference

---

## Features

- **Keypad scan** — drives one row LOW at a time, reads columns, maps to characters via a 4x4 `KEY_MAP`
- **Debounced input** — `waitForKey()` waits for release, press, and release again before returning a character
- **Potentiometer thresholds:**
  - Temperature: 25.0°C — 37.0°C
  - Air quality: 0 — 800 ppm
- **Alarm logic** — fires when LM35 reading exceeds the temperature threshold or MQ-2 detects gas
- **Event log** — 5-slot ring buffer storing temperature, gas state, and cause of each alarm
- **Deactivation code** — `1234` (editable via `DEACTIVATE_CODE` constant)
- **`#` key** — prints the current event log to the serial terminal while idle

---

## How to Run

1. Open `main.cpp` in Keil Studio Cloud, paste in the contents of `Lab5_Code.txt`
2. Select target **NUCLEO-F439ZI** and build
3. Drag the generated `.bin` file onto the NUCLEO drive
4. Open the serial terminal at **9600 baud**
5. Rotate the potentiometer to set thresholds — values update live on the terminal
6. When the alarm triggers, type `1 2 3 4` on the keypad to deactivate
7. Press `#` at any idle moment to view the event log

---

## File Structure

```
Lab5/
├── README.md                  # This file
├── Lab5_Code.txt              # Full source code (main.cpp)
├── Lab5_Logbook_Report.docx   # Submission logbook with reflection
├── videos/
│   └── demo.mp4               # Full system demo
└── images/
    ├── wiring_overview.jpg    # Board, breadboard, keypad, MB102
    ├── keypad_detail.jpg      # Keypad ribbon into breadboard header
    └── serial_output.png      # Serial terminal showing full test flow
```

---

## Key Learnings

- Matrix keypad scanning requires a short settle delay (`wait_us(50)`) between driving a row LOW and reading the columns; without it, presses are missed or misread
- Mbed's `events` namespace already defines a class called `Event`, which caused a naming clash with my original struct — renaming to `AlarmEvent` resolved the build error
- A ring buffer is a much cleaner way to keep the "last N events" than shifting an array every time a new event arrives
- The MB102 module is essential once several 5 V peripherals are on the breadboard — the NUCLEO's onboard regulator struggles to supply enough stable current for the LM35, MQ-2, buzzer, and keypad together

---

## References

- Arm Mbed. *DigitalIn, DigitalOut, AnalogIn, RawSerial* — API documentation. https://os.mbed.com/docs/mbed-os/v5.15/apis/
- Dr. Sunny Atuba. *Lecture 5 — Finite-State Machines and the Real-Time Clock*. School of Engineering, UWE Bristol.
- Dr. Sunny Atuba. *Tutorial Sheet 5*. Embedded Systems module (UFMF11-15-2).
