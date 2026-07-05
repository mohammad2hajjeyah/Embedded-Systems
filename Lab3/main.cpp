#include "mbed.h"
#include "arm_book_lib.h"

DigitalIn switch1(D2);
DigitalIn switch2(D3);
DigitalIn switch3(D4);
DigitalIn switch4(D5);

DigitalOut lightGreen(LED1);
DigitalOut lightBlue(LED2);
DigitalOut lightRed(LED3);

UnbufferedSerial pcTerminal(USBTX, USBRX, 115200);

int getPressedKey();

int main() {
    switch1.mode(PullDown);
    switch2.mode(PullDown);
    switch3.mode(PullDown);
    switch4.mode(PullDown);

    lightGreen = 0;
    lightBlue = 0;
    lightRed = 1;

    int failCounter = 0;
    int afterWarningFlag = 0;

    int userPass[3] = {1, 2, 3};
    int masterPass[3] = {0, 0, 0};
    int typedPass[3] = {-1, -1, -1};

    while (true) {
        int fillCount = 0;

        while (fillCount < 3) {
            int key = getPressedKey();
            if (key != -1) {
                typedPass[fillCount] = key;
                fillCount++;
            }
            delay(10);
        }

        int passMatch = 1;
        for (int i = 0; i < 3; i++) {
            if (typedPass[i] != userPass[i]) {
                passMatch = 0;
            }
        }

        if (passMatch == 1) {
            const char* msg = ">> Access Granted! Device Unlocked.\r\n";
            pcTerminal.write(msg, strlen(msg));
            
            failCounter = 0;
            afterWarningFlag = 0;
            lightRed = 0;
            lightGreen = 1;
            delay(5000);
            lightGreen = 0;
            lightRed = 1;
        } else {
            const char* msg = ">> Access Denied! Wrong Passcode.\r\n";
            pcTerminal.write(msg, strlen(msg));

            if (afterWarningFlag == 1) {
                failCounter = 3;
            } else {
                failCounter++;
            }
            afterWarningFlag = 0;

            if (failCounter == 2) {
                for (int i = 0; i < 10; i++) {
                    lightRed = !lightRed;
                    delay(1000);
                }
                lightRed = 1;
                afterWarningFlag = 1;
            } 
            else if (failCounter >= 3) {
                lightBlue = 1;
                afterWarningFlag = 0;

                int adminDigits = 0;
                int typedAdmin[3] = {-1, -1, -1};
                
                int lockdownTicks = 0;

                while (lockdownTicks < 240) {
                    if (lockdownTicks % 2 == 0) {
                        lightRed = !lightRed;
                    }

                    int immediateKey = -1;
                    if (switch1.read() == 1) immediateKey = 1;
                    else if (switch2.read() == 1) immediateKey = 2;
                    else if (switch3.read() == 1) immediateKey = 3;
                    else if (switch4.read() == 1) immediateKey = 0;

                    if (immediateKey != -1) {
                        delay(20);
                        while (switch1.read() == 1 || switch2.read() == 1 || switch3.read() == 1 || switch4.read() == 1);

                        char admEcho[40];
                        sprintf(admEcho, ">> [LOCKDOWN] Admin Key: %d\r\n", immediateKey);
                        pcTerminal.write(admEcho, strlen(admEcho));

                        typedAdmin[0] = immediateKey;
                        adminDigits = 1;

                        while (adminDigits < 3) {
                            int nextKey = getPressedKey();
                            if (nextKey != -1) {
                                typedAdmin[adminDigits] = nextKey;
                                adminDigits++;
                            }
                            delay(10);
                        }

                        int adminMatch = 1;
                        for (int k = 0; k < 3; k++) {
                            if (typedAdmin[k] != masterPass[k]) {
                                adminMatch = 0;
                            }
                        }

                        if (adminMatch == 1) {
                            failCounter = 0;
                            break;
                        } else {
                            adminDigits = 0;
                        }
                    }

                    delay(500);
                    lockdownTicks++;
                }

                lightBlue = 0;
                lightRed = 1;
            }
        }

        delay(50);
    }
}

int getPressedKey() {
    if (switch1.read() == 1) { 
        delay(20); 
        if (switch1.read() == 1) { 
            while(switch1.read() == 1); 
            const char* echo = "Pressed: Button 1\r\n";
            pcTerminal.write(echo, strlen(echo));
            return 1; 
        } 
    }
    if (switch2.read() == 1) { 
        delay(20); 
        if (switch2.read() == 1) { 
            while(switch2.read() == 1); 
            const char* echo = "Pressed: Button 2\r\n";
            pcTerminal.write(echo, strlen(echo));
            return 2; 
        } 
    }
    if (switch3.read() == 1) { 
        delay(20); 
        if (switch3.read() == 1) { 
            while(switch3.read() == 1); 
            const char* echo = "Pressed: Button 3\r\n";
            pcTerminal.write(echo, strlen(echo));
            return 3; 
        } 
    }
    if (switch4.read() == 1) { 
        delay(20); 
        if (switch4.read() == 1) { 
            while(switch4.read() == 1); 
            const char* echo = "Pressed: Button 0\r\n";
            pcTerminal.write(echo, strlen(echo));
            return 0; 
        } 
    }
    return -1;
}
