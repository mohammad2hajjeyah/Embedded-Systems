// ================================================
// LAB 1 - Creating a Blinky LED
// Mohammad Hajjeyah - 24060397
// ================================================

// ------------------------------------------------
// TASK i - All 3 LEDs blink simultaneously at 500ms
// ------------------------------------------------
#include "mbed.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

int main() {
    led1 = 0; led2 = 0; led3 = 0;
    while (true) {
        led1 = !led1;
        led2 = !led2;
        led3 = !led3;
        thread_sleep_for(500);
    }
}

// ------------------------------------------------
// TASK ii - LEDs blink in sequence (500ms each)
// ------------------------------------------------
#include "mbed.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

int main() {
    led1 = 0; led2 = 0; led3 = 0;
    while (true) {
        led1 = 1; thread_sleep_for(500);
        led1 = 0; thread_sleep_for(500);
        led2 = 1; thread_sleep_for(500);
        led2 = 0; thread_sleep_for(500);
        led3 = 1; thread_sleep_for(500);
        led3 = 0; thread_sleep_for(500);
    }
}

// ------------------------------------------------
// TASK iii - All 3 LEDs blink simultaneously at 300ms
// ------------------------------------------------
#include "mbed.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

int main() {
    led1 = 0; led2 = 0; led3 = 0;
    while (true) {
        led1 = !led1;
        led2 = !led2;
        led3 = !led3;
        thread_sleep_for(300);
    }
}

// ------------------------------------------------
// TASK iv - LED1: 200ms, LED2: 400ms, LED3: 600ms
// ------------------------------------------------
#include "mbed.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

int main() {
    led1 = 0; led2 = 0; led3 = 0;
    while (true) {
        led1 = 1; thread_sleep_for(200);
        led1 = 0;
        led2 = 1; thread_sleep_for(400);
        led2 = 0;
        led3 = 1; thread_sleep_for(600);
        led3 = 0;
    }
}

// ------------------------------------------------
// TASK v - All 3 blink 5 times then LED1 stays on
// ------------------------------------------------
#include "mbed.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

int main() {
    led1 = 0; led2 = 0; led3 = 0;

    for (int i = 0; i < 5; i++) {
        led1 = 1; led2 = 1; led3 = 1;
        thread_sleep_for(200);
        led1 = 0; led2 = 0; led3 = 0;
        thread_sleep_for(200);
    }

    led1 = 1;
    led2 = 0;
    led3 = 0;

    while (true) {
        thread_sleep_for(100);
    }
