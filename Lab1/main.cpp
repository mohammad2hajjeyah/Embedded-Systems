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
}
