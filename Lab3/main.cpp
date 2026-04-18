#include "mbed.h"

RawSerial uartUsb(USBTX, USBRX, 115200);

bool gasAlarm       = false;
bool tempAlarm      = false;
bool monitoringMode = false;

Timer monitorTimer;

void availableCommands() {
    uartUsb.printf("===== SMART HOME SYSTEM =====\r\n");
    uartUsb.printf("Press 1: Toggle gas alarm ON/OFF\r\n");
    uartUsb.printf("Press 2: Get gas alarm state\r\n");
    uartUsb.printf("Press 3: Get temperature alarm state\r\n");
    uartUsb.printf("Press 4: Toggle temperature alarm ON/OFF\r\n");
    uartUsb.printf("Press 5: Reset all alarms\r\n");
    uartUsb.printf("Press 6: Enter/Exit monitoring mode\r\n");
    uartUsb.printf("=============================\r\n\r\n");
}

void sendGasState() {
    if (gasAlarm) {
        uartUsb.printf("GAS ALARM ACTIVE\r\n");
    } else {
        uartUsb.printf("GAS ALARM CLEAR\r\n");
    }
}

void sendTempState() {
    if (tempAlarm) {
        uartUsb.printf("TEMP ALARM ACTIVE\r\n");
    } else {
        uartUsb.printf("TEMP ALARM CLEAR\r\n");
    }
}

void sendStatus() {
    uartUsb.printf("-- STATUS UPDATE --\r\n");
    sendGasState();
    sendTempState();
    uartUsb.printf("\r\n");
}

int main() {
    availableCommands();
    monitorTimer.start();

    while (true) {
        char receivedChar = '\0';

        if (uartUsb.readable()) {
            receivedChar = uartUsb.getc();

            switch (receivedChar) {
                case '1':
                    gasAlarm = !gasAlarm;
                    if (gasAlarm) {
                        uartUsb.printf("WARNING: GAS DETECTED\r\n");
                    } else {
                        uartUsb.printf("Gas alarm deactivated\r\n");
                    }
                    break;

                case '2':
                    sendGasState();
                    break;

                case '3':
                    sendTempState();
                    break;

                case '4':
                    tempAlarm = !tempAlarm;
                    if (tempAlarm) {
                        uartUsb.printf("WARNING: TEMPERATURE TOO HIGH\r\n");
                    } else {
                        uartUsb.printf("Temperature alarm deactivated\r\n");
                    }
                    break;

                case '5':
                    gasAlarm  = false;
                    tempAlarm = false;
                    uartUsb.printf("ALARMS RESET\r\n");
                    break;

                case '6':
                    monitoringMode = !monitoringMode;
                    if (monitoringMode) {
                        uartUsb.printf("Monitoring mode ON\r\n");
                        monitorTimer.reset();
                    } else {
                        uartUsb.printf("Monitoring mode OFF\r\n");
                    }
                    break;

                default:
                    availableCommands();
                    break;
            }
        }

        if (monitoringMode && monitorTimer.read() >= 2.0f) {
            sendStatus();
            monitorTimer.reset();
        }
    }
}
