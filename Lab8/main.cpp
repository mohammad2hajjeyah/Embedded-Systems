#include "mbed.h"
#include "arm_book_lib.h"
#include "FATFileSystem.h"
#include "SDBlockDevice.h"
#include "lcd.h"
#include <cstdio>

AnalogIn dialPot(A0);
AnalogIn tempSensor(A1);
AnalogIn gasSensor(A2);
PwmOut alarmBuzzer(D9);

DigitalOut row1(D0), row2(D1), row3(D2), row4(D3);
DigitalIn col1(D4, PullUp), col2(D5, PullUp), col3(D6, PullUp), col4(D7, PullUp);

SDBlockDevice sdCard(D11, D12, D13, D10);
FATFileSystem fileSystem("sd");

UnbufferedSerial pcPort(USBTX, USBRX, 115200);

char keysLayout[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

char activeMode = 'A';
int isSirenActive = 0;

void initSystemDevices();
void runSafetyMonitoring();
char readKeypad();
void printDefaultLcd();
void saveLogToSd(const char* tag, int reading);
void dumpLogToSerial();

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
    if (activeMode == 'A') {
        lcd_print("Mode: Temp Watch");
        lcd_set(0, 1);
        lcd_print("A:Temp  B:Gas");
    } else {
        lcd_print("Mode: Gas Watch");
        lcd_set(0, 1);
        lcd_print("A:Temp  B:Gas");
    }
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

void saveLogToSd(const char* tag, int reading) {
    sdCard.init();
    fileSystem.mount(&sdCard);
    FILE* dataFile = fopen("/sd/log.txt", "a");
    if (dataFile != NULL) {
        fprintf(dataFile, "Alert: %s reached %d\n", tag, reading);
        fclose(dataFile);
    } else {
        pcPort.write(">> [SD CARD ERROR] Cannot write to storage. Bypass active.\r\n", 59);
    }
    fileSystem.unmount();
    sdCard.deinit();
}

void dumpLogToSerial() {
    sdCard.init();
    fileSystem.mount(&sdCard);
    FILE* dataFile = fopen("/sd/log.txt", "r");
    if (dataFile != NULL) {
        pcPort.write("====== Pulling Event Records From SD Card ======\r\n", 50);
        int singleChar;
        while ((singleChar = fgetc(dataFile)) != EOF) {
            if (singleChar == '\n') {
                pcPort.write("\r\n", 2);
            } else {
                pcPort.write(&singleChar, 1);
            }
        }
        pcPort.write("================ End of File ================\r\n", 47);
        fclose(dataFile);
    } else {
        pcPort.write(">> [SD CARD] No Storage File Found or Empty.\r\n", 46);
    }
    fileSystem.unmount();
    sdCard.deinit();
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
        if (freezeTimer == 0 && isSirenActive == 0) {
            printDefaultLcd();
        }
    }

    if (pressedKey == 'A' || pressedKey == 'B') {
        if (isSirenActive == 0) {
            activeMode = pressedKey;
            char modeMsg[50];
            sprintf(modeMsg, ">> [SYSTEM] Log Scope Shifted: %s\r\n", (activeMode == 'A') ? "TEMPERATURE" : "GAS");
            pcPort.write(modeMsg, strlen(modeMsg));
            printDefaultLcd();
        }
    }

    if (pressedKey == '*') {
        if (isSirenActive == 0) {
            dumpLogToSerial();
        }
    }

    int tempFault = (tempCelsius > safetyLimit) ? 1 : 0;
    int gasFault = (gasLevel > (safetyLimit * 1.5)) ? 1 : 0;

    int shouldAlarmTrigger = 0;
    if (activeMode == 'A' && tempFault) shouldAlarmTrigger = 1;
    if (activeMode == 'B' && gasFault) shouldAlarmTrigger = 1;

    if (shouldAlarmTrigger && isSirenActive == 0 && freezeTimer == 0) {
        isSirenActive = 1;
        
        alarmBuzzer.period(1.0 / 2000.0);
        alarmBuzzer.write(0.5);
        
        lcd_clear();
        lcd_print("!! WARNING !!");
        lcd_set(0, 1);
        
        if (activeMode == 'A') {
            lcd_print("Temp Threshold");
            const char* msg1 = "!! WARNING !! Danger: Over Temperature Detected!\r\n";
            pcPort.write(msg1, strlen(msg1));
            saveLogToSd("Temperature", (int)tempCelsius);
        } else {
            lcd_print("Gas Leakage");
            const char* msg1 = "!! WARNING !! Danger: Gas Leak Detected!\r\n";
            pcPort.write(msg1, strlen(msg1));
            saveLogToSd("Gas", (int)gasLevel);
        }
    }

    if (isSirenActive == 1) {
        if (pressedKey == '#') {
            isSirenActive = 0;
            alarmBuzzer.write(0.0);
            
            lcd_clear();
            lcd_print("Alert Dismissed");
            lcd_set(0, 1);
            lcd_print("Resuming Monitor");
            
            freezeTimer = 300;
        }
    } else {
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
