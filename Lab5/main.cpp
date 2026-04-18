#include "mbed.h"
#include <stdio.h>
#include <string.h>

RawSerial pc(USBTX, USBRX);

AnalogIn  pot(A0);
AnalogIn  lm35(A1);
DigitalIn gasDOUT(PE_12);
DigitalOut buzzer(PE_10);

DigitalOut row1(PB_3);
DigitalOut row2(PB_5);
DigitalOut row3(PC_7);
DigitalOut row4(PA_15);

DigitalIn col1(PB_12);
DigitalIn col2(PB_13);
DigitalIn col3(PB_15);
DigitalIn col4(PC_6);

const char KEY_MAP[4][4] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

const char DEACTIVATE_CODE[4] = { '1', '2', '3', '4' };

struct AlarmEvent {
    float temperature;
    int   gasLevel;
    char  cause[16];
};
AlarmEvent eventLog[5];
int eventCount = 0;

bool alarmActive = false;

float readTempThreshold() {
    return 25.0f + (pot.read() * 12.0f);
}

int readGasThreshold() {
    return (int)(pot.read() * 800.0f);
}

float readTemperature() {
    return lm35.read() * 165.0f;
}

char scanKeypad() {
    col1.mode(PullUp);
    col2.mode(PullUp);
    col3.mode(PullUp);
    col4.mode(PullUp);

    DigitalOut* rows[4] = { &row1, &row2, &row3, &row4 };
    DigitalIn*  cols[4] = { &col1, &col2, &col3, &col4 };

    for (int i = 0; i < 4; i++) {
        rows[i]->write(1);
    }

    for (int r = 0; r < 4; r++) {
        rows[r]->write(0);
        wait_us(50);
        for (int c = 0; c < 4; c++) {
            if (cols[c]->read() == 0) {
                rows[r]->write(1);
                return KEY_MAP[r][c];
            }
        }
        rows[r]->write(1);
    }
    return 0;
}

char waitForKey() {
    while (scanKeypad() != 0) {
        thread_sleep_for(10);
    }
    char k = 0;
    while (k == 0) {
        k = scanKeypad();
        thread_sleep_for(20);
    }
    while (scanKeypad() != 0) {
        thread_sleep_for(10);
    }
    return k;
}

void logEvent(float temp, int gas, const char* cause) {
    int slot = eventCount % 5;
    eventLog[slot].temperature = temp;
    eventLog[slot].gasLevel    = gas;
    strncpy(eventLog[slot].cause, cause, sizeof(eventLog[slot].cause) - 1);
    eventLog[slot].cause[sizeof(eventLog[slot].cause) - 1] = '\0';
    eventCount++;
}

void printEventLog() {
    pc.printf("\r\n========= EVENT LOG (last 5) =========\r\n");
    if (eventCount == 0) {
        pc.printf("No events recorded.\r\n");
    } else {
        int shown = (eventCount < 5) ? eventCount : 5;
        int start = (eventCount < 5) ? 0 : (eventCount % 5);
        for (int i = 0; i < shown; i++) {
            int idx = (start + i) % 5;
            pc.printf("  #%d  Cause: %-11s  Temp: %.1fC  Gas: %s\r\n",
                      i + 1,
                      eventLog[idx].cause,
                      eventLog[idx].temperature,
                      eventLog[idx].gasLevel ? "Detected" : "None");
        }
    }
    pc.printf("======================================\r\n\r\n");
}

bool enterDeactivationCode() {
    char entered[4];
    pc.printf("Enter 4-Digit Code to Deactivate: ");
    for (int i = 0; i < 4; i++) {
        char k = waitForKey();
        while (k < '0' || k > '9') {
            k = waitForKey();
        }
        entered[i] = k;
        pc.printf("%c", k);
    }
    pc.printf("\r\n");

    for (int i = 0; i < 4; i++) {
        if (entered[i] != DEACTIVATE_CODE[i]) {
            return false;
        }
    }
    return true;
}

int main() {
    gasDOUT.mode(PullUp);
    buzzer = 1;

    pc.baud(9600);
    pc.printf("\r\n=== Lab 5: Matrix Keypad Smart Home System ===\r\n");
    pc.printf("Press '#' on the keypad at any time to view the event log.\r\n\r\n");

    while (true) {

        float temp        = readTemperature();
        float tempThresh  = readTempThreshold();
        int   gasThresh   = readGasThreshold();
        bool  gasDetected = (gasDOUT.read() == 0);

        bool tempWarning = (temp > tempThresh);
        bool gasWarning  = gasDetected;

        if ((tempWarning || gasWarning) && !alarmActive) {
            alarmActive = true;
            const char* cause = "Temp+Gas";
            if (tempWarning && !gasWarning) {
                cause = "Temperature";
            } else if (gasWarning && !tempWarning) {
                cause = "Gas";
            }
            logEvent(temp, gasDetected ? 1 : 0, cause);
            pc.printf("*** ALARM TRIGGERED - Cause: %s ***\r\n", cause);
        }

        buzzer = alarmActive ? 0 : 1;

        pc.printf("Temp: %.1fC | T-Thresh: %.1fC | Gas: %s | G-Thresh: %dppm | Alarm: %s\r\n",
                  temp, tempThresh,
                  gasDetected ? "DETECTED" : "None",
                  gasThresh,
                  alarmActive ? "ON" : "OFF");

        if (alarmActive) {
            if (enterDeactivationCode()) {
                pc.printf(">>> Correct code. Alarm deactivated.\r\n\r\n");
                alarmActive = false;
                buzzer = 1;
            } else {
                pc.printf(">>> Wrong code. Alarm stays active.\r\n");
            }
        } else {
            char k = scanKeypad();
            if (k == '#') {
                printEventLog();
                while (scanKeypad() != 0) {
                    thread_sleep_for(10);
                }
            }
            thread_sleep_for(1000);
        }
    }
}
