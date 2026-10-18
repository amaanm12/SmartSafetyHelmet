#pragma once

#include <Arduino.h>

// Runs Amaan's serial fall simulation and the local touch, buzzer, and LED
// alarm. Keeping it in a class lets main.cpp stay short and lets Wi-Fi read
// the alarm state without owning the alarm hardware.
class AlarmSystem {
public:
    void begin();
    void update();
    bool fallDetected() const;
    bool sosActive() const;
    const char* stateName() const;

private:
    enum class State : uint8_t { Normal, FallPending, SosActive };

    void startFallCountdown();
    void startSos();
    void cancelFall();
    void acknowledgeSos();
    void reset();
    void readSerialCommand();
    void readTouchInput(uint32_t now);
    void updateCountdown(uint32_t now);
    void updateOutputs(uint32_t now);

    State state_ = State::Normal;
    uint32_t fallStartedAt_ = 0;
    uint32_t touchStartedAt_ = 0;
    uint32_t lastCountdownAt_ = 0;
    uint32_t lastLedToggleAt_ = 0;
    bool touchReleased_ = false;
    bool ledOn_ = false;
};
