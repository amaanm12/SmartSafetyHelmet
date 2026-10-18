#include <Arduino.h>
#include <WiFi.h>
#include "sensor.h"
#include "fall_model.h"

// ESP32-WROOM-32 default I2C pins: SDA = GPIO 21, SCL = GPIO 22.
IMUSensor imu;
constexpr size_t WINDOW_SIZE = 100;
constexpr size_t STRIDE = 20;

static float ring[100][6];
static float inputWindow[100][6];
static size_t writeIndex = 0;
static size_t buffered = 0;
static size_t sinceVerdict = 0;

bool updateFallModel(float ax, float ay, float az,
                     float gx, float gy, float gz,
                     float& score, bool& fallEvent) {
    ring[writeIndex][0] = ax; ring[writeIndex][1] = ay; ring[writeIndex][2] = az;
    ring[writeIndex][3] = gx; ring[writeIndex][4] = gy; ring[writeIndex][5] = gz;
    writeIndex = (writeIndex + 1) % 100;
    if (buffered < 100) {
        ++buffered;
        if (buffered < 100) return false;
    } else {
        if (++sinceVerdict < 20) return false;
    }
    sinceVerdict = 0;
    for (size_t i = 0; i < 100; ++i)
        for (size_t c = 0; c < 6; ++c)
            inputWindow[i][c] = ring[(writeIndex+i)%100][c];
    score = fall_detector::probability(inputWindow);
    fallEvent = score >= fall_detector::THRESHOLD;
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    WiFi.mode(WIFI_STA);
    if (!imu.begin(21, 22)) {
        Serial.println("IMU initialization failed. Check the wiring.");
        while (true) {
            delay(1000);
        }
    }
}

void loop() {
    imu.update();
    IMUData r = imu.getData();
    float score;
    bool fallEvent;
    if (updateFallModel(r.accX, r.accY, r.accZ,
                        r.gyroX, r.gyroY, r.gyroZ, score, fallEvent)) {
        Serial.printf("%lu,%.6f,%s\n", millis(), score,
                      fallEvent ? "FALL_EVENT" : "NO_FALL");
    }
    delay(10);
}
