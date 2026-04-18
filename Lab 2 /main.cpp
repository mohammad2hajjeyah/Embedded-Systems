#include "mbed.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

DigitalIn btn0(D2);
DigitalIn btn1(D3);
DigitalIn btn2(D4);
DigitalIn btn3(D5);
DigitalIn btn4(D6);
DigitalIn btn5(D7);

const int USER_CODE[4]  = {0, 1, 2, 3};
const int ADMIN_CODE[4] = {5, 4, 3, 2};

enum State { NORMAL, WARNING, LOCKDOWN };
State systemState = NORMAL;

int wrongAttempts  = 0;
int lockdownCount  = 0;

int readDigit() {
    btn0.mode(PullDown); btn1.mode(PullDown);
    btn2.mode(PullDown); btn3.mode(PullDown);
    btn4.mode(PullDown); btn5.mode(PullDown);
    if (btn0) return 0;
    if (btn1) return 1;
    if (btn2) return 2;
    if (btn3) return 3;
    if (btn4) return 4;
    if (btn5) return 5;
    return -1;
}

int waitForPress() {
    while (readDigit() != -1) { thread_sleep_for(10); }
    int d = -1;
    while (d == -1) { d = readDigit(); thread_sleep_for(20); }
    while (readDigit() != -1) { thread_sleep_for(10); }
    return d;
}

void readCode(int code[4]) {
    for (int i = 0; i < 4; i++) {
        code[i] = waitForPress();
        thread_sleep_for(100);
    }
}

bool codesMatch(const int a[4], const int b[4]) {
    for (int i = 0; i < 4; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

void blinkEventLog() {
    for (int i = 0; i < lockdownCount; i++) {
        led3 = 1; thread_sleep_for(200);
        led3 = 0; thread_sleep_for(200);
    }
}

int main() {
    led1 = 0; led2 = 0; led3 = 0;

    while (true) {

        if (systemState == NORMAL) {
            led1 = 0; led2 = 0;
            int entered[4];
            readCode(entered);
            if (codesMatch(entered, USER_CODE)) {
                wrongAttempts = 0;
            } else {
                wrongAttempts++;
                if (wrongAttempts >= 3) {
                    systemState = WARNING;
                }
            }
        }

        else if (systemState == WARNING) {
            int elapsed = 0;
            while (elapsed < 30000) {
                led1 = 1; thread_sleep_for(1000);
                led1 = 0; thread_sleep_for(1000);
                elapsed += 2000;
            }
            led1 = 0;
            int entered[4];
            readCode(entered);
            if (codesMatch(entered, USER_CODE)) {
                wrongAttempts = 0;
                systemState   = NORMAL;
            } else {
                lockdownCount++;
                systemState = LOCKDOWN;
            }
        }

        else if (systemState == LOCKDOWN) {
            led1 = 1;
            blinkEventLog();
            int elapsed = 0;
            bool adminUnlocked = false;
            while (elapsed < 60000 && !adminUnlocked) {
                led2 = 1; thread_sleep_for(300);
                led2 = 0; thread_sleep_for(700);
                elapsed += 1000;
                if (readDigit() != -1) {
                    int entered[4];
                    readCode(entered);
                    if (codesMatch(entered, ADMIN_CODE)) {
                        adminUnlocked = true;
                    }
                }
            }
            led1 = 0; led2 = 0;
            wrongAttempts = 0;
            systemState   = NORMAL;
        }
    }
}
