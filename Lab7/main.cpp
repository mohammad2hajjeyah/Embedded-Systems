#include "mbed.h"
#include "arm_book_lib.h"
#include "lcd.h"

DigitalOut motorRelay(D8);
DigitalIn stopButton(D9);

DigitalOut row1(D0), row2(D1), row3(D2), row4(D3);
DigitalIn col1(D4, PullUp), col2(D5, PullUp), col3(D6, PullUp), col4(D7, PullUp);

UnbufferedSerial pcPort(USBTX, USBRX, 115200);

char keysLayout[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

void initSystem();
void checkControlLogic();
char readKeypad();
void updateLcdStatus(const char* line1, const char* line2);

int main() {
    initSystem();
    while (true) {
        checkControlLogic();
    }
}

void initSystem() {
    motorRelay = 0;
    stopButton.mode(PullDown);
    
    lcd_init();
    updateLcdStatus("System Ready", "Motor: STOPPED");
}

void updateLcdStatus(const char* line1, const char* line2) {
    lcd_clear();
    lcd_print(line1);
    lcd_set(0, 1);
    lcd_print(line2);
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

void checkControlLogic() {
    static int loopCounter = 0;
    
    char pressedKey = readKeypad();

    if (pressedKey == 'A') {
        motorRelay = 1;
        updateLcdStatus("Motor Running", "Status: ACTIVE");
        const char* msg = ">> Motor Activated via Keypad\r\n";
        pcPort.write(msg, strlen(msg));
    }
    else if (pressedKey == 'B') {
        motorRelay = 0;
        updateLcdStatus("System Neutral", "Motor: STOPPED");
        const char* msg = ">> Motor Deactivated via Keypad\r\n";
        pcPort.write(msg, strlen(msg));
    }

    if (stopButton.read() == 1) {
        motorRelay = 0;
        updateLcdStatus("!! EMERGENCY !!", "Motor: OVERRIDE");
        const char* msg = ">> EMERGENCY STOP: Push Button Pressed!\r\n";
        pcPort.write(msg, strlen(msg));
        while(stopButton.read() == 1);
    }

    loopCounter++;
    if (loopCounter >= 100) {
        char statusFrame[80];
        sprintf(statusFrame, "Current State | Relay: %d | Stop Button: %d\r\n", motorRelay.read(), stopButton.read());
        pcPort.write(statusFrame, strlen(statusFrame));
        loopCounter = 0;
    }

    delay(10);
}
