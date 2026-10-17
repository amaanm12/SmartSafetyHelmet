#pragma once

#include "sensor.h"

// Keeps all Wi-Fi and network details out of main.cpp. begin() connects the
// ESP32 to Wi-Fi and starts its web API; update() gives that API fresh readings
// and lets it answer browser requests.
class WifiCommunication {
public:
    void begin();
    void update(const IMUData& sensorData, bool sosActive);
};
