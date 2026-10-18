#include <Arduino.h>

#include "alarm.h"
#include "detection.h"
#include "sensor.h"
#include "wifi_communication.h"

namespace {
// These pins match the ESP32-WROOM-32 wiring in README.md.
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;

IMUSensor imu;
WifiCommunication wifi;
bool imuReady = false;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    // Start the touch sensor, buzzer and LED first so the alarm works even
    // if the MPU6050 is missing.
    alarmBegin();

    // setup() runs once after the ESP32 powers on or resets.
    imuReady = imu.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    if (!imuReady) {
        Serial.println("MPU6050 initialization failed. Check the wiring. Continuing without IMU data.");
    }

    // Connect to Wi-Fi and start the web API. The dashboard's "Call worker"
    // button runs alarmCallWorker() to beep the buzzer, and "Trigger SOS" runs
    // onFallDetected() to start the 15 second countdown.
    wifi.begin(alarmCallWorker, onFallDetected);
}

void loop() {
    // Keep the alarm and Wi-Fi running around the teammate's sampling loop.
    alarmUpdate();

    if (imuReady) {
        imu.update();
        IMUData r = imu.getData();
        float score;
        bool fallEvent;
        if (updateFallModel(r.accX, r.accY, r.accZ,
                            r.gyroX, r.gyroY, r.gyroZ, score, fallEvent)) {
            if(fallEvent) {
                Serial.println("FALL DETECTED");
                // Connect his detection result to the existing helmet alarm.
                onFallDetected();
            }
        }
    }

    wifi.update(imu.getData(), {alarmIsSosActive(), alarmIsCallActive(), alarmSosCountdownSeconds()});
    delay(10);
}
