Each lab folder contains:
- `code/` — the main source file(s)
- `videos/` — video evidence of each task
- `screenshots/` — LCD photos and serial terminal captures

---

## Lab 6 — I2C LCD Smart Home System

### Overview

This lab extends the smart home system from Lab 3 by adding a **20x4 character LCD** over the **I2C bus** through a **PCF8574 I/O expander**. The system is controlled via a **4x4 matrix keypad** and reports gas, temperature, and alarm state on the LCD.

Implementation satisfies all four requirements of **Main Task 6**:

1. Keys **4** and **5** show gas and temperature state on the LCD.
2. Alarm state auto-refreshes on the LCD every **60 seconds**.
3. A **warning** is displayed when temperature or gas levels exceed safe limits.
4. A **5-digit deactivation code** (`13709`) entered via the keypad silences the alarm, with a startup prompt ("Enter Code to Deactivate Alarm").

### Hardware

| Component | NUCLEO pin |
|---|---|
| LCD SDA (PCF8574) | PB_9 (D14) |
| LCD SCL (PCF8574) | PB_8 (D15) |
| LCD VCC / GND | 5V / GND |
| LM35 temperature sensor | A1 |
| Gas sensor (digital) | PD_2 |
| Buzzer | PE_10 |
| Keypad rows | PB_3, PB_5, PC_7, PA_15 |
| Keypad columns | PB_12, PB_13, PB_15, PC_6 |

**I2C address:** `0x4E` (8-bit write address, confirmed with I2C scanner)
**I2C frequency:** 100 kHz

### Software design

The firmware is organised around a **state machine** for both the LCD screens and the keypad scanner to keep I2C traffic clean and avoid display corruption.

**LCD screens:**
- `SCREEN_PROMPT` — default; shows the deactivation-code prompt and any warnings
- `SCREEN_GAS_REPORT` — shown for 3 s when key 4 is pressed
- `SCREEN_TEMP_REPORT` — shown for 3 s when key 5 is pressed
- `SCREEN_ALARM_STATUS` — briefly shown every 60 s

**Keypad states:** `SCANNING → DEBOUNCE → HELD`, returning each key character once on **release** (prevents repeat triggers).

**Deactivation code:** `13709`

### Build and flash

1. Open the project in Keil Studio Cloud.
2. Copy the contents of `Lab6/code/Lab6_Code.txt` into `main.cpp`.
3. Build → drag the `.bin` file onto the NUCLEO drive.
4. Open the built-in serial terminal at **115200 baud** to view keypress logs.

### Test procedure

| Step | Action | Expected result |
|---|---|---|
| 1 | Power on | LCD shows "Enter Code to / Deactivate Alarm:" |
| 2 | Press key **4** | LCD shows gas detector state for 3 s |
| 3 | Press key **5** | LCD shows temperature reading for 3 s |
| 4 | Trigger gas sensor | "!! GAS DETECTED" appears on line 4, buzzer sounds |
| 5 | Enter **13709** | "Alarm Deactivated" appears, buzzer silences |
| 6 | Enter wrong code | "Wrong code, retry" appears, prompt returns |
| 7 | Wait 60 s | Alarm status screen briefly displays |

### Key technical notes

- The LCD uses 4-bit mode on the PCF8574 backpack, with `D4–D7` on bits 4–7 and `RS / RW / EN / Backlight` on bits 0–3.
- Each enable pulse holds `E` high for **2 ms**, giving the cheaper I2C backpack enough margin to latch data reliably.
- Background tasks (sensor polling, LCD refresh, alarm-state refresh) run off counters incremented by a 10 ms tick, so no blocking waits interfere with I2C traffic.
- A 3-state keypad scanner with 40 ms debounce ensures each physical keypress produces exactly one event.

---
