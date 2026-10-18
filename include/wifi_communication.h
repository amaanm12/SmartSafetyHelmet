#pragma once

#include "sensor.h"

// Called when a dashboard button asks the helmet to do something ("Call worker"
// or "Trigger SOS"). Returns true if it started, false if the helmet is busy.
using AlarmCommandHandler = bool (*)();

// The alarm state reported to the dashboard in /api/data.
struct AlarmStatus {
    bool sosActive;
    bool callActive;
    unsigned long sosCountdownS;  // 0 when no fall countdown is running
};

// Keeps all Wi-Fi and network details out of main.cpp. begin() connects the
// ESP32 to Wi-Fi and starts its web API; update() gives that API fresh readings
// and lets it answer browser requests.
class WifiCommunication {
public:
    void begin(AlarmCommandHandler onCallWorker, AlarmCommandHandler onTriggerSos);
    void update(const IMUData& sensorData, const AlarmStatus& alarmStatus);
};
