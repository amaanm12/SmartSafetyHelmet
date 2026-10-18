#pragma once

#include "sensor.h"

// Keeps all Wi-Fi and network details out of main.cpp. begin() connects the
// ESP32 to Wi-Fi in the background; update() starts the API once connected,
// gives it fresh readings, and lets it answer browser requests.
class WifiCommunication {
public:
    void begin();
    void update(const IMUData& sensorData, bool imuAvailable, bool fallDetected,
                bool sosActive, const char* alarmState);
};
