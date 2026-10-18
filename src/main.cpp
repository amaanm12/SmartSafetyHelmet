#include <Arduino.h>

#include "alarm.h"
#include "sensor.h"
#include "wifi_communication.h"

namespace {
// These pins match the ESP32-WROOM-32 wiring in README.md.
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
IMUSensor imu;
AlarmSystem alarmSystem;
WifiCommunication wifi;
bool imuAvailable = false;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[BOOT] ShieldGuard serial-fall firmware starting.");

    // The MPU6050 pins do not overlap the alarm pins. It is optional for this
    // serial fall demo: Wi-Fi and alarms still run if the MPU6050 is unplugged.
    imuAvailable = imu.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    if (!imuAvailable) {
        Serial.println("MPU6050 not found; continuing with IMU readings disabled.");
    }

    // GPIO 27 is used by Amaan's touch sensor, so the older SOS button reader
    // is not started in this demo build.
    alarmSystem.begin();
    wifi.begin();
    Serial.println("[READY] Serial commands enabled: f = fall, r = reset. Wi-Fi connects in background.");
}

void loop() {
    // Type f in Serial Monitor to simulate a fall. Alarm state and readings are
    // passed to the Wi-Fi API so the dashboard can show the same event.
    if (imuAvailable) imu.update();
    alarmSystem.update();
    wifi.update(imu.getData(), imuAvailable, alarmSystem.fallDetected(),
                alarmSystem.sosActive(), alarmSystem.stateName());
    delay(10);
}
