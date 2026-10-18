#include <Arduino.h>

#include "alarm.h"
#include "sensor.h"
#include "wifi_communication.h"

namespace {
// These pins match the ESP32-WROOM-32 wiring in README.md.
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr uint32_t IMU_SAMPLE_INTERVAL_MS = 100;

IMUSensor imu;
WifiCommunication wifi;
bool imuReady = false;
uint32_t lastImuSampleAt = 0;
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
    // loop() repeats continuously: keep the alarm responsive, sample the IMU
    // every 100 ms, and let the web server answer browser requests.
    alarmUpdate();

    const uint32_t now = millis();
    if (imuReady && now - lastImuSampleAt >= IMU_SAMPLE_INTERVAL_MS) {
        lastImuSampleAt = now;
        imu.update();
    }

    wifi.update(imu.getData(), {alarmIsSosActive(), alarmIsCallActive(), alarmSosCountdownSeconds()});
    delay(10);
}
