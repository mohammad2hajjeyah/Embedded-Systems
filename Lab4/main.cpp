#include "mbed.h"

DigitalOut myLed1(LED1);
DigitalOut myLed2(LED2);
DigitalOut myLed3(LED3);

void doInitialBlink();

int main() {
    doInitialBlink();

    int timePass = 0;
    int firstLedCounter = 0;
    
    int internalT2 = 0;
    int internalT1 = 0;

    while (firstLedCounter < 8) {
        myLed3 = !myLed3;

        internalT2++;
        if (internalT2 >= 2) {
            myLed2 = !myLed2;
            internalT2 = 0;
        }

        internalT1++;
        if (internalT1 >= 4) {
            myLed1 = !myLed1;
            firstLedCounter++;
            internalT1 = 0;
        }

        timePass++;
        ThisThread::sleep_for(250);
    }

    myLed1 = 1;
    myLed2 = 0;
    myLed3 = 0;

    while (true) {
        ThisThread::sleep_for(1000);
    }
}

void doInitialBlink() {
    int runTimes = 0;
    while (runTimes < 4) {
        myLed1 = 1; myLed2 = 1; myLed3 = 1;
        ThisThread::sleep_for(100);
        myLed1 = 0; myLed2 = 0; myLed3 = 0;
        ThisThread::sleep_for(100);
        runTimes++;
    }
}
