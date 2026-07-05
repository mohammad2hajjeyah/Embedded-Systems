#include "mbed.h"
#include "arm_book_lib.h"
#include "lcd.h"

AnalogIn dialPot(A0);
AnalogIn tempSensor(A1);
AnalogIn gasSensor(A2);
PwmOut alarmBuzzer(D9);

DigitalOut row1(D0), row2(D1), row3(D2), row4(D3);
DigitalIn col1(D4, PullUp), col2(D5, PullUp), col3(D6, PullUp), col4(D7, PullUp);

UnbufferedSerial pcPort(USBTX, USBRX, 115200);

char keysLayout[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

char savedPass[3] = {'\0', '\0', '\0'};
int savedPassLen = 0;
int isLocked = 0;

void initSystemDevices();
void runSafetyMonitoring();
char readKeypad();
void printDefaultLcd();

int main() {
    initSystemDevices();
    while (true) {
        runSafetyMonitoring();
    }
}

void initSystemDevices() {
    alarmBuzzer.write(0.0);
    lcd_init();
    printDefaultLcd();
}

void printDefaultLcd() {
    lcd_clear();
    lcd_print("Enter Code to");
    lcd_set(0, 1);
    lcd_print("Deactivate Alarm");
}

char readKeypad() {
    DigitalOut* rows[4] = {&row1, &row2, &row3, &row4};
    DigitalIn* cols[4] = {&col1, &col2, &col3, &col4};
    
    for (int r = 0; r < 4; r++) {
        row1 = row2 = row3 = row4 = 1;
        *rows[r] = 0;
        
        for (int c = 0; c < 4; c++) {
            if (cols[c]->read() == 0) {
                delay(40);
                if (cols[c]->read() == 0) {
                    while (cols[c]->read() == 0);
                    char foundKey = keysLayout[r][c];
                    
                    char echoFrame[25];
                    sprintf(echoFrame, "KEY: %c\r\n", foundKey);
                    pcPort.write(echoFrame, strlen(echoFrame));
                    
                    return foundKey;
                }
            }
        }
    }
    return '\0';
}

void runSafetyMonitoring() {
    static int loopCounter = 0;
    static int freezeTimer = 0;

    float potReading = dialPot.read();
    float safetyLimit = potReading * 60.0;

    float currentVoltage = tempSensor.read() * 3.3;
    float tempCelsius = currentVoltage * 100.0;

    float gasLevel = gasSensor.read() * 100.0;

    char pressedKey = readKeypad();

    if (freezeTimer > 0) {
        freezeTimer--;
        if (freezeTimer == 0 && isLocked == 0) {
            printDefaultLcd();
        }
    }

    int tempFault = (tempCelsius > safetyLimit) ? 1 : 0;
    int gasFault = (gasLevel > (safetyLimit * 1.5)) ? 1 : 0;

    if ((tempFault || gasFault) && isLocked == 0 && freezeTimer == 0) {
        isLocked = 1;
        savedPassLen = 0;
        
        alarmBuzzer.period(1.0 / 2000.0);
        alarmBuzzer.write(0.5);
        
        lcd_clear();
        lcd_print("!! WARNING !!");
        lcd_set(0, 1);
        if (tempFault && gasFault) lcd_print("Temp & Gas Leak");
        else if (tempFault) lcd_print("Over Temperature");
        else lcd_print("Gas Threat");

        const char* msg1 = "!! WARNING !! Danger: Environmental Fault Detected!\r\n";
        pcPort.write(msg1, strlen(msg1));
        const char* msg2 = "Enter 3-Digit Code to Deactivate\r\n";
        pcPort.write(msg2, strlen(msg2));

        while (isLocked == 1) {
            char lockKey = readKeypad();
            if (lockKey != '\0') {
                if (lockKey == '*') {
                    if (savedPassLen == 3 && savedPass[0] == '1' && savedPass[1] == '2' && savedPass[2] == '3') {
                        isLocked = 0;
                        alarmBuzzer.write(0.0);
                        savedPassLen = 0;
                        
                        lcd_clear();
                        lcd_print("System Cleared");
                        lcd_set(0, 1);
                        lcd_print("Alarm Silenced");
                        
                        const char* success = "Code Correct. Resetting System Status...\r\n";
                        pcPort.write(success, strlen(success));
                        freezeTimer = 300;
                    } else {
                        savedPassLen = 0;
                        lcd_clear();
                        lcd_print("Invalid Token");
                        lcd_set(0, 1);
                        lcd_print("Try Again...");
                        
                        const char* fail = "Wrong Code! Resetting...\r\n";
                        pcPort.write(fail, strlen(fail));
                        
                        delay(1500);
                        
                        lcd_clear();
                        lcd_print("!! WARNING !!");
                        lcd_set(0, 1);
                        if (tempFault) lcd_print("Over Temperature");
                        else lcd_print("Gas Threat");
                    }
                } else if (lockKey != '#' && savedPassLen < 3) {
                    savedPass[savedPassLen] = lockKey;
                    savedPassLen++;
                }
            }
            delay(10);
        }
    } else {
        alarmBuzzer.write(0.0);

        if (isLocked == 0) {
            if (pressedKey == '2') {
                lcd_clear();
                lcd_print("Gas Status:");
                lcd_set(0, 1);
                char gasStr[16];
                sprintf(gasStr, "Level: %d", (int)gasLevel);
                lcd_print(gasStr);
                freezeTimer = 300;
            } 
            else if (pressedKey == '3') {
                lcd_clear();
                lcd_print("Temp Status:");
                lcd_set(0, 1);
                char tempStr[16];
                sprintf(tempStr, "Value: %d C", (int)tempCelsius);
                lcd_print(tempStr);
                freezeTimer = 300;
            }
        }

        loopCounter++;
        if (loopCounter >= 100) {
            int tWhole = (int)tempCelsius;
            int tFract = (int)((tempCelsius - tWhole) * 10.0);
            if (tFract < 0) tFract = -tFract;

            int limWhole = (int)safetyLimit;
            int limFract = (int)((safetyLimit - limWhole) * 10.0);
            if (limFract < 0) limFract = -limFract;

            char statusFrame[160];
            sprintf(statusFrame, "Safe Status | Room Temp: %d.%d C | Gas Index: %d | Guard Point: %d.%d C\r\n", 
                    tWhole, tFract, (int)gasLevel, limWhole, limFract);
            
            pcPort.write(statusFrame, strlen(statusFrame));
            loopCounter = 0;
        }
    }

    delay(10);
}
