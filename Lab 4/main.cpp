#include "mbed.h"

AnalogIn   tempSensor(A1);
AnalogIn   potentiometer(A0);
DigitalIn  gasSensor(D8, PullUp);
DigitalOut buzzer(PE_10);

RawSerial  pc(USBTX, USBRX, 9600);

#define TEMP_SCALE       165.0f
#define TEMP_THRESH_MIN  20.0f
#define TEMP_THRESH_MAX  70.0f
#define POLL_INTERVAL_MS 1000

#define BUZZER_ON   0
#define BUZZER_OFF  1

float readTemperature() {
    return tempSensor.read() * TEMP_SCALE;
}

float readThreshold() {
    float pot = potentiometer.read();
    return TEMP_THRESH_MIN + pot * (TEMP_THRESH_MAX - TEMP_THRESH_MIN);
}

bool gasDetected() {
    return (gasSensor.read() == 0);
}

int main() {
    buzzer = BUZZER_OFF;

    pc.printf("\r\n=== Lab 4: Sensor Monitoring System ===\r\n");
    pc.printf("Potentiometer sets temperature threshold (20C - 70C)\r\n");
    pc.printf("MQ-2 warming up... allow ~60s for stable readings.\r\n\r\n");

    while (true) {
        float temperature = readTemperature();
        float threshold   = readThreshold();
        bool  gas         = gasDetected();

        bool tempWarning = (temperature > threshold);
        bool gasWarning  = gas;

        if (tempWarning && gasWarning) {
            buzzer = BUZZER_ON;
            pc.printf("Buzzer ON - Cause: Temperature AND Gas | Temp: %.1fC | Threshold: %.1fC\r\n",
                      temperature, threshold);
        } else if (tempWarning) {
            buzzer = BUZZER_ON;
            pc.printf("Buzzer ON - Cause: Temperature | Temp: %.1fC | Threshold: %.1fC\r\n",
                      temperature, threshold);
        } else if (gasWarning) {
            buzzer = BUZZER_ON;
            pc.printf("Buzzer ON - Cause: Gas | Temp: %.1fC | Threshold: %.1fC\r\n",
                      temperature, threshold);
        } else {
            buzzer = BUZZER_OFF;
            pc.printf("System Normal | Temp: %.1fC | Threshold: %.1fC | Gas: None\r\n",
                      temperature, threshold);
        }

        wait_us(POLL_INTERVAL_MS * 1000);
    }
}
