// ================================================
// LAB 6 - I2C LCD Smart Home System
// Mohammad Hajjeyah - 24060397
// ================================================

#include "mbed.h"

// ── I2C LCD (PCF8574 backpack) ────────────────────────────
#define PCF8574_ADDR       (0x27 << 1)

#define LCD_RS             0x01
#define LCD_RW             0x02
#define LCD_EN             0x04
#define LCD_BL             0x08
#define LCD_D4             0x10
#define LCD_D5             0x20
#define LCD_D6             0x40
#define LCD_D7             0x80

// ── Timing ───────────────────────────────────────────────
#define TICK_MS                 10
#define DEBOUNCE_MS             40
#define LCD_REFRESH_MS          300
#define ALARM_REFRESH_MS      60000
#define REPORT_DURATION_MS     3000

// ── Limits and code ──────────────────────────────────────
#define TEMP_LIMIT_C            40.0f
#define NUMBER_OF_KEYS           5

// ── Keypad ───────────────────────────────────────────────
#define KEYPAD_ROWS              4
#define KEYPAD_COLS              4

// ── Peripherals ──────────────────────────────────────────
I2C         i2cLcd(PB_9, PB_8);
DigitalIn   gasSensor(PD_2);
AnalogIn    tempSensor(A1);
DigitalOut  buzzer(PE_10);
RawSerial   uartUsb(USBTX, USBRX, 115200);

DigitalOut keypadRowPins[KEYPAD_ROWS] = { PB_3, PB_5, PC_7, PA_15 };
DigitalIn  keypadColPins[KEYPAD_COLS] = { PB_12, PB_13, PB_15, PC_6 };

const char keypadMap[KEYPAD_ROWS * KEYPAD_COLS] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

// ── System state ─────────────────────────────────────────
typedef enum {
    KEYPAD_SCANNING,
    KEYPAD_DEBOUNCE,
    KEYPAD_HELD
} keypadState_t;

typedef enum {
    SCREEN_PROMPT,
    SCREEN_GAS_REPORT,
    SCREEN_TEMP_REPORT,
    SCREEN_ALARM_STATUS
} screen_t;

bool  gasAlarm      = false;
bool  tempAlarm     = false;
float currentTempC  = 22.0f;

const char codeSequence[NUMBER_OF_KEYS] = { '1', '3', '7', '0', '9' };
char       keyPressed[NUMBER_OF_KEYS]   = { 0 };
int        codeIndex                    = 0;

keypadState_t keypadState      = KEYPAD_SCANNING;
char          lastKeyDetected  = '\0';
int           debounceCounter  = 0;

screen_t currentScreen         = SCREEN_PROMPT;
int      lcdRefreshCounter     = 0;
int      alarmRefreshCounter   = 0;
int      reportCounter         = 0;

// =========================================================
//  LCD DRIVER
// =========================================================
void lcdWriteByte(uint8_t data) {
    char buf = data | LCD_BL;
    i2cLcd.write(PCF8574_ADDR, &buf, 1);
}

void lcdSendNibble(uint8_t nibble, bool rs) {
    uint8_t data = 0;
    if (nibble & 0x01) data |= LCD_D4;
    if (nibble & 0x02) data |= LCD_D5;
    if (nibble & 0x04) data |= LCD_D6;
    if (nibble & 0x08) data |= LCD_D7;
    if (rs)            data |= LCD_RS;

    lcdWriteByte(data | LCD_EN);
    wait_ms(2);
    lcdWriteByte(data);
    wait_ms(2);
}

void lcdSendByte(uint8_t byte, bool rs) {
    lcdSendNibble(byte >> 4, rs);
    lcdSendNibble(byte & 0x0F, rs);
}

void lcdCommand(uint8_t cmd) { lcdSendByte(cmd, false); }
void lcdChar   (char    c  ) { lcdSendByte((uint8_t)c, true); }

void lcdInit() {
    i2cLcd.frequency(100000);
    wait_ms(100);

    lcdSendNibble(0x03, false); wait_ms(5);
    lcdSendNibble(0x03, false); wait_ms(1);
    lcdSendNibble(0x03, false); wait_ms(1);
    lcdSendNibble(0x02, false); wait_ms(1);

    lcdCommand(0x28);
    lcdCommand(0x0C);
    lcdCommand(0x06);
    lcdCommand(0x01);
    wait_ms(2);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
    static const uint8_t rowOffsets[] = { 0x00, 0x40, 0x14, 0x54 };
    lcdCommand(0x80 | (col + rowOffsets[row]));
}

void lcdClear() {
    lcdCommand(0x01);
    wait_ms(2);
}

void lcdPrint(const char* str) {
    while (*str) lcdChar(*str++);
}

// =========================================================
//  KEYPAD (3-state debounced scanner)
// =========================================================
void keypadInit() {
    for (int i = 0; i < KEYPAD_COLS; i++) keypadColPins[i].mode(PullUp);
    for (int i = 0; i < KEYPAD_ROWS; i++) keypadRowPins[i] = 1;
}

char keypadScan() {
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        for (int i = 0; i < KEYPAD_ROWS; i++) keypadRowPins[i] = 1;
        keypadRowPins[row] = 0;
        for (int col = 0; col < KEYPAD_COLS; col++) {
            if (keypadColPins[col] == 0) {
                return keypadMap[row * KEYPAD_COLS + col];
            }
        }
    }
    return '\0';
}

char keypadUpdate() {
    char detected = '\0';
    char released = '\0';

    switch (keypadState) {
        case KEYPAD_SCANNING:
            detected = keypadScan();
            if (detected != '\0') {
                lastKeyDetected = detected;
                debounceCounter = 0;
                keypadState     = KEYPAD_DEBOUNCE;
            }
            break;

        case KEYPAD_DEBOUNCE:
            if (debounceCounter >= DEBOUNCE_MS) {
                detected = keypadScan();
                keypadState = (detected == lastKeyDetected)
                              ? KEYPAD_HELD : KEYPAD_SCANNING;
            }
            debounceCounter += TICK_MS;
            break;

        case KEYPAD_HELD:
            detected = keypadScan();
            if (detected != lastKeyDetected) {
                if (detected == '\0') released = lastKeyDetected;
                keypadState = KEYPAD_SCANNING;
            }
            break;
    }
    return released;
}

// =========================================================
//  SENSOR READING
// =========================================================
void readTemperature() {
    float voltage = tempSensor.read() * 3.3f;
    float reading = voltage * 100.0f;
    if (reading >= 0.0f && reading <= 100.0f) {
        currentTempC = reading;
    }
}

void checkSafetyLimits() {
    if (currentTempC > TEMP_LIMIT_C) tempAlarm = true;
    if (gasSensor == 1)              gasAlarm  = true;
    buzzer = (gasAlarm || tempAlarm) ? 1 : 0;
}

// =========================================================
//  SCREEN DRAWING
// =========================================================
void screenShowPrompt() {
    lcdSetCursor(0, 0); lcdPrint("Enter Code to       ");
    lcdSetCursor(0, 1); lcdPrint("Deactivate Alarm:   ");

    char codeLine[21] = "                    ";
    for (int i = 0; i < codeIndex; i++) codeLine[i] = '*';
    lcdSetCursor(0, 2); lcdPrint(codeLine);

    lcdSetCursor(0, 3);
    if      (gasAlarm)  lcdPrint("!! GAS DETECTED     ");
    else if (tempAlarm) lcdPrint("!! TEMP TOO HIGH    ");
    else                lcdPrint("                    ");
}

void screenShowGasReport() {
    lcdSetCursor(0, 0); lcdPrint("-- GAS DETECTOR --  ");
    lcdSetCursor(0, 1);
    lcdPrint(gasAlarm ? "State: ALARM ACTIVE " : "State: CLEAR        ");
    lcdSetCursor(0, 2); lcdPrint("                    ");
    lcdSetCursor(0, 3); lcdPrint("                    ");
}

void screenShowTempReport() {
    char line[21];
    lcdSetCursor(0, 0); lcdPrint("-- TEMPERATURE --   ");
    snprintf(line, sizeof(line), "Temp: %.1f C %s      ",
             currentTempC, tempAlarm ? "HI" : "OK");
    lcdSetCursor(0, 1); lcdPrint(line);
    lcdSetCursor(0, 2); lcdPrint("                    ");
    lcdSetCursor(0, 3); lcdPrint("                    ");
}

void screenShowAlarmStatus() {
    lcdSetCursor(0, 0); lcdPrint("-- ALARM STATUS --  ");
    lcdSetCursor(0, 1);
    lcdPrint((gasAlarm || tempAlarm) ? "Alarm: ACTIVE       "
                                     : "Alarm: OFF          ");
    lcdSetCursor(0, 2); lcdPrint("                    ");
    lcdSetCursor(0, 3); lcdPrint("                    ");
}

void lcdUpdate() {
    switch (currentScreen) {
        case SCREEN_PROMPT:       screenShowPrompt();      break;
        case SCREEN_GAS_REPORT:   screenShowGasReport();   break;
        case SCREEN_TEMP_REPORT:  screenShowTempReport();  break;
        case SCREEN_ALARM_STATUS: screenShowAlarmStatus(); break;
        default:                  screenShowPrompt();      break;
    }
}

// =========================================================
//  CODE CHECK
// =========================================================
bool codesMatch() {
    for (int i = 0; i < NUMBER_OF_KEYS; i++) {
        if (keyPressed[i] != codeSequence[i]) return false;
    }
    return true;
}

void serialPrint(const char* msg) {
    uartUsb.printf("%s", msg);
}

// =========================================================
//  MAIN
// =========================================================
int main() {
    keypadInit();
    lcdInit();
    lcdClear();

    serialPrint("Lab 6 - I2C LCD Smart Home System\r\n");

    while (true) {
        readTemperature();
        checkSafetyLimits();

        char key = keypadUpdate();
        if (key != '\0') {
            uartUsb.printf("Key pressed: %c\r\n", key);

            if (key == '4') {
                currentScreen = SCREEN_GAS_REPORT;
                reportCounter = 0;
            }
            else if (key == '5') {
                currentScreen = SCREEN_TEMP_REPORT;
                reportCounter = 0;
            }
            else if (key >= '0' && key <= '9' && codeIndex < NUMBER_OF_KEYS) {
                keyPressed[codeIndex++] = key;
                currentScreen = SCREEN_PROMPT;

                if (codeIndex >= NUMBER_OF_KEYS) {
                    if (codesMatch()) {
                        gasAlarm  = false;
                        tempAlarm = false;
                        buzzer    = 0;
                        lcdClear();
                        lcdSetCursor(0, 0); lcdPrint("Alarm Deactivated   ");
                        wait_ms(2000);
                        lcdClear();
                        serialPrint("Code correct. Alarm deactivated.\r\n");
                    } else {
                        lcdClear();
                        lcdSetCursor(0, 0); lcdPrint("Wrong code, retry   ");
                        wait_ms(2000);
                        lcdClear();
                        serialPrint("Incorrect code.\r\n");
                    }
                    codeIndex = 0;
                    for (int i = 0; i < NUMBER_OF_KEYS; i++) keyPressed[i] = 0;
                    currentScreen = SCREEN_PROMPT;
                }
            }
        }

        if (currentScreen == SCREEN_GAS_REPORT ||
            currentScreen == SCREEN_TEMP_REPORT) {
            reportCounter += TICK_MS;
            if (reportCounter >= REPORT_DURATION_MS) {
                reportCounter = 0;
                currentScreen = SCREEN_PROMPT;
                lcdClear();
            }
        }

        lcdRefreshCounter += TICK_MS;
        if (lcdRefreshCounter >= LCD_REFRESH_MS) {
            lcdRefreshCounter = 0;
            lcdUpdate();
        }

        alarmRefreshCounter += TICK_MS;
        if (alarmRefreshCounter >= ALARM_REFRESH_MS) {
            alarmRefreshCounter = 0;
            screen_t saved = currentScreen;
            currentScreen  = SCREEN_ALARM_STATUS;
            lcdUpdate();
            wait_ms(2000);
            lcdClear();
            currentScreen = saved;
        }

        wait_ms(TICK_MS);
    }
}
