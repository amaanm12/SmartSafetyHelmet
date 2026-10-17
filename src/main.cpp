#include <Arduino.h>
#include <WiFi.h>
#include "sensor.h"

// ESP32-WROOM-32 default I2C pins: SDA = GPIO 21, SCL = GPIO 22.
IMUSensor imu;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("ESP32-WROOM-32 is working!");

    WiFi.mode(WIFI_STA);
    Serial.println("Scanning WiFi networks...");

    // Set up the IMU module on the ESP32 I2C pins.
    if (!imu.begin(21, 22)) {
        Serial.println("IMU initialization failed. Check the wiring.");
        while (true) {
            delay(1000);
        }
    }
}

void loop() {
    /*
    int networks = WiFi.scanNetworks();

    Serial.printf("Found %d networks\n", networks);

    for (int i = 0; i < networks; i++) {
        Serial.println(WiFi.SSID(i));
    }
        */

   // delay(5000);
    // Read the latest sample from the MPU6050.
    imu.update();

    IMUData sensorReadings = imu.getData();

    Serial.printf("Acceleration [m/s^2]  X: %+7.2f  Y: %+7.2f  Z: %+7.2f\n",
                  sensorReadings.accX,
                  sensorReadings.accY,
                  sensorReadings.accZ);
    Serial.printf("Gyroscope [rad/s]     X: %+7.2f  Y: %+7.2f  Z: %+7.2f\n",
                  sensorReadings.gyroX,
                  sensorReadings.gyroY,
                  sensorReadings.gyroZ);
    Serial.printf("Temperature           %5.1f C\n", sensorReadings.temp);
    Serial.println("-----------------------------------------------");
    delay(100);
}
