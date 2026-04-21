// ================================================================
// LAB 7 - DC Motor Driving using Relays and Interrupts
// Integrated Smart Home System with Motor, PIR and Gate Control
// Mohammad Hajjeyah - 24060397
// Module: Embedded Systems (UFMF11-15-2) - Dr. Sunny Atuba
// ================================================================

#include "mbed.h"

// ==========================================================
//  Compile-time constants
// ==========================================================
#define I2C_ADDR           0x4E      // PCF8574 I2C address (8-bit)
#define LCD_BACKLIGHT      0x08
#define LCD_ENABLE         0x04
#define LCD_RW             0x02
#define LCD_RS             0x01

#define DEACTIVATION_CODE  "13709"
#define CODE_LENGTH        5

#define TEMP_THRESHOLD     30.0f     // degrees Celsius

// Loop timing - main tick is 10 ms
#define TICK_MS              10
#define LCD_REFRESH_TICKS    30      // 300 ms
#define REPORT_TIMEOUT_TICKS 300     // 3  s
#define MOTOR_TICKS          10      // 100 ms motor FSM update
#define SENSOR_TICKS         100     // 1  s sensor poll
#define ALARM_REFRESH_TICKS  6000    // 60 s alarm status refresh

// ==========================================================
//  Peripheral declarations
// ==========================================================

// Serial + I2C LCD
RawSerial   pc(USBTX, USBRX, 115200);
I2C         lcdBus(PB_9, PB_8);

// 4x4 Matrix Keypad (carry-over from Lab 5/6)
DigitalOut  row1(PB_3),  row2(PB_5),  row3(PC_7),  row4(PA_15);
DigitalIn   col1(PB_12), col2(PB_13), col3(PB_15), col4(PC_6);

// Sensors and buzzer (carry-over from Lab 3/4/6)
AnalogIn    tempSensor(A1);         // LM35
DigitalIn   gasSensor(PD_2);        // MQ-2 DOUT
DigitalOut  buzzer(PE_10);

// Motor relay control - DigitalInOut so pins can be configured
// as high-impedance when the relay should be de-energised
DigitalInOut motorM1Pin(PF_2);      // Relay IN1
DigitalInOut motorM2Pin(PE_3);      // Relay IN2

// Direction buttons + limit switches (interrupt driven, PullUp)
InterruptIn dir1Button(PF_9);       // Open  / Direction 1
InterruptIn dir2Button(PF_7);       // Close / Direction 2
InterruptIn dir1LimitSwitch(PG_3);  // Limit switch 1
InterruptIn dir2LimitSwitch(PF_8);  // Limit switch 2

// PIR motion sensor (interrupt driven, rising edge)
InterruptIn pirSensor(PG_0);

// Direction LEDs
DigitalOut  gateOpenLed(D8);        // Green - opening
DigitalOut  gateCloseLed(D9);       // Red   - closing

// ==========================================================
//  State types
// ==========================================================
enum motorDirection_t { DIRECTION_1, DIRECTION_2, MOTOR_STOPPED };
enum gateStatus_t     { GATE_CLOSED, GATE_OPEN, GATE_OPENING, GATE_CLOSING };
enum ScreenState      { SCREEN_PROMPT, SCREEN_GAS, SCREEN_TEMP,
                        SCREEN_ALARM,  SCREEN_GATE, SCREEN_INTRUDER };

// Shared state - volatile because ISRs write to these
volatile motorDirection_t motorDirection = MOTOR_STOPPED;
volatile motorDirection_t motorState     = MOTOR_STOPPED;
volatile gateStatus_t     gateStatus     = GATE_CLOSED;
volatile ScreenState      screenState    = SCREEN_PROMPT;

// Limit-switch latches - gate starts closed, so dir2LS is active
volatile bool dir1LSLatched = false;
volatile bool dir2LSLatched = true;

// Alarm + event flags set by ISRs and sensor poll, cleared by main
volatile bool intruderAlarm = false;
volatile bool gasAlarm      = false;
volatile bool tempAlarm     = false;
volatile bool pirEvent      = false;

// Keypad entry buffer
char enteredCode[CODE_LENGTH + 1];
int  codeIndex = 0;

// Latest sensor values
float latestTemp = 22.0f;
bool  latestGas  = false;

// ==========================================================
//  Keypad map - rows/cols follow Lab 5 wiring
// ==========================================================
const char KEY_MAP[4][4] = {
    { '1','2','3','A' },
    { '4','5','6','B' },
    { '7','8','9','C' },
    { '*','0','#','D' }
};

// ==========================================================
//  LCD helpers - PCF8574 I2C backpack, 4-bit mode
// ==========================================================
void lcdPulseEnable(char data) {
    char hi = data | LCD_ENABLE;
    char lo = data & ~LCD_ENABLE;
    lcdBus.write(I2C_ADDR, &hi, 1);
    wait_us(2000);                  // 2 ms enable high - reliable on cheap boards
    lcdBus.write(I2C_ADDR, &lo, 1);
    wait_us(100);
}

void lcdSendNibble(char nibble, char mode) {
    char data = (nibble & 0xF0) | LCD_BACKLIGHT | mode;
    lcdBus.write(I2C_ADDR, &data, 1);
    lcdPulseEnable(data);
}

void lcdSendByte(char value, char mode) {
    lcdSendNibble(value & 0xF0, mode);
    lcdSendNibble((value << 4) & 0xF0, mode);
    wait_us(100);
}

void lcdCommand(char c) { lcdSendByte(c, 0); }
void lcdData(char c)    { lcdSendByte(c, LCD_RS); }

void lcdInit() {
    wait_ms(50);
    // Force 4-bit mode via the classic HD44780 sequence
    lcdSendNibble(0x30, 0); wait_ms(5);
    lcdSendNibble(0x30, 0); wait_us(200);
    lcdSendNibble(0x30, 0); wait_us(200);
    lcdSendNibble(0x20, 0); wait_us(200);

    lcdCommand(0x28);   // 4-bit, 2 lines, 5x8 font
    lcdCommand(0x0C);   // display on, cursor off
    lcdCommand(0x06);   // entry mode - increment, no shift
    lcdCommand(0x01);   // clear
    wait_ms(5);
}

void lcdSetCursor(int col, int row) {
    static const int offs[] = { 0x00, 0x40, 0x14, 0x54 };
    lcdCommand(0x80 | (col + offs[row]));
}

void lcdPrint(const char* s) {
    while (*s) lcdData(*s++);
}

void lcdPrintLine(int row, const char* text) {
    lcdSetCursor(0, row);
    char buf[21];
    int i = 0;
    while (text[i] && i < 20) { buf[i] = text[i]; i++; }
    while (i < 20) { buf[i++] = ' '; }
    buf[20] = '\0';
    lcdPrint(buf);
}

// ==========================================================
//  Motor control - Example 7.1 style FSM
//  Runs every 100 ms via motorTicks counter in main loop
// ==========================================================
void motorControlInit() {
    motorM1Pin.mode(OpenDrain);
    motorM2Pin.mode(OpenDrain);
    motorM1Pin.input();
    motorM2Pin.input();
    motorDirection = MOTOR_STOPPED;
    motorState     = MOTOR_STOPPED;
}

void motorDirectionWrite(motorDirection_t d) {
    motorDirection = d;
}

motorDirection_t motorDirectionRead() {
    return motorDirection;
}

void motorControlUpdate() {
    switch (motorState) {
        case DIRECTION_1:
            if (motorDirection == DIRECTION_2 ||
                motorDirection == MOTOR_STOPPED) {
                motorM1Pin.input();
                motorM2Pin.input();
                motorState = MOTOR_STOPPED;
            }
            break;

        case DIRECTION_2:
            if (motorDirection == DIRECTION_1 ||
                motorDirection == MOTOR_STOPPED) {
                motorM1Pin.input();
                motorM2Pin.input();
                motorState = MOTOR_STOPPED;
            }
            break;

        case MOTOR_STOPPED:
        default:
            if (motorDirection == DIRECTION_1) {
                motorM2Pin.input();
                motorM1Pin.output();
                motorM1Pin = 0;
                motorState = DIRECTION_1;
            }
            if (motorDirection == DIRECTION_2) {
                motorM1Pin.input();
                motorM2Pin.output();
                motorM2Pin = 0;
                motorState = DIRECTION_2;
            }
            break;
    }

    // Direction LEDs mirror the motor state
    gateOpenLed  = (motorState == DIRECTION_1) ? 1 : 0;
    gateCloseLed = (motorState == DIRECTION_2) ? 1 : 0;
}

// ==========================================================
//  Gate control - Example 7.2 style wrapper
// ==========================================================
void gateOpen() {
    if (!dir1LSLatched) {
        motorDirectionWrite(DIRECTION_1);
        gateStatus     = GATE_OPENING;
        dir2LSLatched  = false;
    }
}

void gateClose() {
    if (!dir2LSLatched) {
        motorDirectionWrite(DIRECTION_2);
        gateStatus     = GATE_CLOSING;
        dir1LSLatched  = false;
    }
}

// ==========================================================
//  Interrupt service routines (kept short - flags only)
// ==========================================================
void dir1ButtonISR() {
    gateOpen();
    screenState = SCREEN_GATE;
}

void dir2ButtonISR() {
    gateClose();
    screenState = SCREEN_GATE;
}

void dir1LSISR() {
    if (motorState == DIRECTION_1) {
        motorDirectionWrite(MOTOR_STOPPED);
        gateStatus    = GATE_OPEN;
        dir1LSLatched = true;
    }
}

void dir2LSISR() {
    if (motorState == DIRECTION_2) {
        motorDirectionWrite(MOTOR_STOPPED);
        gateStatus    = GATE_CLOSED;
        dir2LSLatched = true;
    }
}

void pirISR() {
    pirEvent      = true;
    intruderAlarm = true;
    screenState   = SCREEN_INTRUDER;
}

// ==========================================================
//  Keypad scanning - 3-state debounced, returns on release
// ==========================================================
enum KeypadState { SCANNING, DEBOUNCE, HELD };
KeypadState kpState = SCANNING;
char        kpLast  = 0;
int         kpHoldTicks = 0;

char scanKeypad() {
    DigitalOut* rows[4] = { &row1, &row2, &row3, &row4 };
    col1.mode(PullUp); col2.mode(PullUp);
    col3.mode(PullUp); col4.mode(PullUp);

    for (int r = 0; r < 4; r++) {
        *rows[0] = 1; *rows[1] = 1; *rows[2] = 1; *rows[3] = 1;
        *rows[r] = 0;
        wait_us(50);
        int c = -1;
        if (!col1) c = 0;
        else if (!col2) c = 1;
        else if (!col3) c = 2;
        else if (!col4) c = 3;
        if (c != -1) {
            *rows[0] = 1; *rows[1] = 1; *rows[2] = 1; *rows[3] = 1;
            return KEY_MAP[r][c];
        }
    }
    return 0;
}

// Returns a character once, on release, with debouncing
char getKeyRelease() {
    char raw = scanKeypad();
    switch (kpState) {
        case SCANNING:
            if (raw != 0) { kpLast = raw; kpHoldTicks = 0; kpState = DEBOUNCE; }
            break;
        case DEBOUNCE:
            kpHoldTicks++;
            if (kpHoldTicks >= 4) {            // 40 ms debounce
                if (raw == kpLast)  kpState = HELD;
                else                kpState = SCANNING;
            }
            break;
        case HELD:
            if (raw == 0) {
                kpState = SCANNING;
                return kpLast;                 // fire on release
            }
            break;
    }
    return 0;
}

// ==========================================================
//  LCD screen rendering
// ==========================================================
void drawScreen() {
    char line[21];

    switch (screenState) {
        case SCREEN_PROMPT:
            lcdPrintLine(0, "Smart Home System");
            lcdPrintLine(1, "Enter Code to");
            lcdPrintLine(2, "Deactivate Alarm:");
            // Line 3 shows any active warnings
            if (intruderAlarm)       lcdPrintLine(3, "!! INTRUDER ALERT");
            else if (gasAlarm)       lcdPrintLine(3, "!! GAS DETECTED");
            else if (tempAlarm)      lcdPrintLine(3, "!! OVER TEMPERATURE");
            else                     lcdPrintLine(3, "System OK");
            break;

        case SCREEN_GAS:
            lcdPrintLine(0, "-- GAS DETECTOR --");
            lcdPrintLine(1, latestGas ? "State: GAS DETECTED"
                                      : "State: CLEAR");
            lcdPrintLine(2, "");
            lcdPrintLine(3, "");
            break;

        case SCREEN_TEMP:
            lcdPrintLine(0, "-- TEMPERATURE --");
            sprintf(line, "Temp: %.1f C", latestTemp);
            lcdPrintLine(1, line);
            lcdPrintLine(2, latestTemp > TEMP_THRESHOLD ? "Status: HIGH"
                                                       : "Status: OK");
            lcdPrintLine(3, "");
            break;

        case SCREEN_ALARM:
            lcdPrintLine(0, "-- ALARM STATUS --");
            lcdPrintLine(1, (intruderAlarm||gasAlarm||tempAlarm)
                              ? "ALARM: ACTIVE" : "ALARM: IDLE");
            sprintf(line, "Temp: %.1f C", latestTemp);
            lcdPrintLine(2, line);
            lcdPrintLine(3, latestGas ? "Gas: DETECTED" : "Gas: CLEAR");
            break;

        case SCREEN_GATE:
            lcdPrintLine(0, "-- GATE STATUS --");
            switch (gateStatus) {
                case GATE_CLOSED:  lcdPrintLine(1, "State: CLOSED");  break;
                case GATE_OPEN:    lcdPrintLine(1, "State: OPEN");    break;
                case GATE_OPENING: lcdPrintLine(1, "State: OPENING"); break;
                case GATE_CLOSING: lcdPrintLine(1, "State: CLOSING"); break;
            }
            switch (motorState) {
                case DIRECTION_1:   lcdPrintLine(2, "Motor: DIR 1");   break;
                case DIRECTION_2:   lcdPrintLine(2, "Motor: DIR 2");   break;
                case MOTOR_STOPPED: lcdPrintLine(2, "Motor: STOPPED"); break;
            }
            lcdPrintLine(3, "");
            break;

        case SCREEN_INTRUDER:
            lcdPrintLine(0, "!! INTRUDER ALERT !!");
            lcdPrintLine(1, "PIR motion detected");
            lcdPrintLine(2, "Alarm ACTIVE");
            lcdPrintLine(3, "Enter code to clear");
            break;
    }
}

// ==========================================================
//  Code entry + alarm deactivation
// ==========================================================
void resetCodeBuffer() {
    for (int i = 0; i <= CODE_LENGTH; i++) enteredCode[i] = 0;
    codeIndex = 0;
}

bool codeMatches() {
    for (int i = 0; i < CODE_LENGTH; i++) {
        if (enteredCode[i] != DEACTIVATION_CODE[i]) return false;
    }
    return true;
}

void handleKeyPress(char k) {
    switch (k) {
        case '4':
            screenState = SCREEN_GAS;
            return;
        case '5':
            screenState = SCREEN_TEMP;
            return;
        case 'A':
            gateOpen();
            screenState = SCREEN_GATE;
            return;
        case 'B':
            gateClose();
            screenState = SCREEN_GATE;
            return;
        default:
            break;
    }

    // Digit entry for the deactivation code
    if (k >= '0' && k <= '9' && codeIndex < CODE_LENGTH) {
        enteredCode[codeIndex++] = k;
        if (codeIndex == CODE_LENGTH) {
            if (codeMatches()) {
                pc.printf("Code correct. Alarm deactivated.\r\n");
                intruderAlarm = false;
                gasAlarm      = false;
                tempAlarm     = false;
                buzzer        = 0;
                lcdPrintLine(0, "Alarm Deactivated");
                lcdPrintLine(1, "");
                lcdPrintLine(2, "");
                lcdPrintLine(3, "");
                wait_ms(1500);
            } else {
                pc.printf("Incorrect code.\r\n");
                lcdPrintLine(0, "Wrong code, retry");
                lcdPrintLine(1, "");
                lcdPrintLine(2, "");
                lcdPrintLine(3, "");
                wait_ms(1500);
            }
            resetCodeBuffer();
            screenState = SCREEN_PROMPT;
        }
    }
}

// ==========================================================
//  Sensor polling + alarm evaluation (runs every 1 s)
// ==========================================================
void pollSensors() {
    // LM35 - scale factor calibrated for the 5 V-supply rig
    float v  = tempSensor.read() * 3.3f;
    float t  = v * 100.0f;
    if (t < 0.0f || t > 100.0f) t = latestTemp;  // reject noise
    latestTemp = t;

    latestGas  = (gasSensor.read() == 1);

    if (latestTemp > TEMP_THRESHOLD) tempAlarm = true;
    if (latestGas)                   gasAlarm  = true;
}

// ==========================================================
//  Serial command handling ('m' motor, 'g' gate, 'h' help)
// ==========================================================
void pcSerialUpdate() {
    if (!pc.readable()) return;
    char c = pc.getc();

    switch (c) {
        case 'm': case 'M':
            switch (motorDirectionRead()) {
                case MOTOR_STOPPED:
                    pc.printf("The motor is stopped\r\n"); break;
                case DIRECTION_1:
                    pc.printf("The motor is turning in direction 1\r\n"); break;
                case DIRECTION_2:
                    pc.printf("The motor is turning in direction 2\r\n"); break;
            }
            break;
        case 'g': case 'G':
            switch (gateStatus) {
                case GATE_CLOSED:  pc.printf("The gate is closed\r\n"); break;
                case GATE_OPEN:    pc.printf("The gate is open\r\n");   break;
                case GATE_OPENING: pc.printf("The gate is opening\r\n");break;
                case GATE_CLOSING: pc.printf("The gate is closing\r\n");break;
            }
            break;
        case 'h': case 'H':
            pc.printf("Commands: m/M motor, g/G gate, h/H help\r\n");
            break;
        default:
            break;
    }
}

// ==========================================================
//  Main
// ==========================================================
int main() {
    // I2C + LCD
    lcdBus.frequency(100000);
    lcdInit();

    // Sensor / buzzer defaults
    buzzer = 0;
    gasSensor.mode(PullNone);

    // LEDs off at boot
    gateOpenLed  = 0;
    gateCloseLed = 0;

    // Motor module init (both relay pins high-Z)
    motorControlInit();

    // Interrupt buttons - internal pull-up, trigger on falling edge
    dir1Button.mode(PullUp);      dir1Button.fall(&dir1ButtonISR);
    dir2Button.mode(PullUp);      dir2Button.fall(&dir2ButtonISR);
    dir1LimitSwitch.mode(PullUp); dir1LimitSwitch.fall(&dir1LSISR);
    dir2LimitSwitch.mode(PullUp); dir2LimitSwitch.fall(&dir2LSISR);

    // PIR rises HIGH on motion (HC-SR501 active-high output)
    pirSensor.mode(PullNone);
    pirSensor.rise(&pirISR);

    // Keypad rows default high
    row1 = 1; row2 = 1; row3 = 1; row4 = 1;

    resetCodeBuffer();

    pc.printf("\r\n=== Lab 7 - Motor, PIR, Gate Smart Home System ===\r\n");
    pc.printf("Student: Mohammad Hajjeyah - 24060397\r\n");
    pc.printf("Commands: m/M motor status, g/G gate status, h/H help\r\n");
    pc.printf("PIR warming up (60s), false triggers may occur...\r\n\r\n");

    // Initial screen
    screenState = SCREEN_PROMPT;
    drawScreen();

    // Main loop counters
    int tickLcd    = 0;
    int tickReport = 0;
    int tickMotor  = 0;
    int tickSensor = 0;
    int tickAlarm  = 0;

    while (true) {

        // --- Keypad scan every tick ---------------------------
        char k = getKeyRelease();
        if (k) {
            pc.printf("Key pressed: %c\r\n", k);
            handleKeyPress(k);
        }

        // --- Motor FSM every 100 ms ---------------------------
        if (++tickMotor >= MOTOR_TICKS) {
            tickMotor = 0;
            motorControlUpdate();
        }

        // --- Sensor poll every 1 s ----------------------------
        if (++tickSensor >= SENSOR_TICKS) {
            tickSensor = 0;
            pollSensors();
        }

        // --- LCD refresh every 300 ms -------------------------
        if (++tickLcd >= LCD_REFRESH_TICKS) {
            tickLcd = 0;
            drawScreen();
        }

        // --- Return from report screens after 3 s -------------
        if (screenState == SCREEN_GAS  || screenState == SCREEN_TEMP ||
            screenState == SCREEN_GATE || screenState == SCREEN_INTRUDER) {
            if (++tickReport >= REPORT_TIMEOUT_TICKS) {
                tickReport  = 0;
                screenState = SCREEN_PROMPT;
            }
        } else {
            tickReport = 0;
        }

        // --- Alarm status broadcast every 60 s ----------------
        if (++tickAlarm >= ALARM_REFRESH_TICKS) {
            tickAlarm = 0;
            pc.printf("[STATUS] Temp=%.1fC Gas=%d Alarm=%d Gate=%d Motor=%d\r\n",
                      latestTemp, latestGas ? 1 : 0,
                      (intruderAlarm||gasAlarm||tempAlarm) ? 1 : 0,
                      gateStatus, motorState);
        }

        // --- Drive buzzer on any active alarm -----------------
        buzzer = (intruderAlarm || gasAlarm || tempAlarm) ? 1 : 0;

        // --- Serial command updates ---------------------------
        pcSerialUpdate();

        // --- Log PIR event once -------------------------------
        if (pirEvent) {
            pirEvent = false;
            pc.printf(">>> PIR TRIGGERED - Intruder alarm active\r\n");
        }

        wait_ms(TICK_MS);
    }
}
