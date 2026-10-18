#include "alarm.h"

namespace {
// Pin assignments from Amaan's alarm implementation. GPIO 21 and 22 remain
// available for the MPU6050 I2C bus; the alarm uses 25, 26, and 27.
constexpr uint8_t TOUCH_PIN = 27;
constexpr uint8_t BUZZER_PIN = 25;
constexpr uint8_t RED_LED_PIN = 26;

// This matches Amaan's active-high touch sensor and active-low buzzer module.
constexpr uint8_t TOUCH_ACTIVE_LEVEL = HIGH;
constexpr uint8_t BUZZER_ON_LEVEL = LOW;
constexpr uint8_t BUZZER_OFF_LEVEL = HIGH;

constexpr uint32_t FALL_CANCEL_TIME_MS = 15000;
constexpr uint32_t TOUCH_HOLD_TIME_MS = 2000;
constexpr uint32_t LED_FLASH_INTERVAL_MS = 500;
}  // namespace

void AlarmSystem::begin() {
    // The touch module drives HIGH when touched; a pull-down keeps an
    // unconnected input safely LOW during the serial-only demo.
    pinMode(TOUCH_PIN, INPUT_PULLDOWN);

    // Set safe OFF levels before switching the output pins on.
    digitalWrite(BUZZER_PIN, BUZZER_OFF_LEVEL);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, LOW);
    pinMode(RED_LED_PIN, OUTPUT);

    Serial.println();
    Serial.println("ShieldGuard alarm ready.");
    Serial.println("Type f then Enter to simulate a fall.");
    Serial.println("Type r then Enter to reset the alarm.");
    Serial.println("During an alarm, hold the touch sensor for 2 seconds to cancel or silence.");
}

void AlarmSystem::update() {
    readSerialCommand();
    // Read time after a serial command starts its timer. Otherwise a command
    // arriving across a millisecond boundary can make elapsed time underflow.
    const uint32_t now = millis();
    readTouchInput(now);
    updateCountdown(now);
    updateOutputs(millis());
}

bool AlarmSystem::fallDetected() const {
    return state_ != State::Normal;
}

bool AlarmSystem::sosActive() const {
    return state_ == State::SosActive;
}

const char* AlarmSystem::stateName() const {
    switch (state_) {
        case State::FallPending: return "FALL_PENDING";
        case State::SosActive: return "SOS_ACTIVE";
        case State::Normal:
        default: return "NORMAL";
    }
}

void AlarmSystem::startFallCountdown() {
    if (state_ != State::Normal) {
        Serial.println("Fall command received; an alarm is already pending or active. Type r to reset.");
        return;
    }

    state_ = State::FallPending;
    fallStartedAt_ = millis();
    touchStartedAt_ = 0;
    lastCountdownAt_ = 0;
    touchReleased_ = false;
    ledOn_ = false;
    digitalWrite(BUZZER_PIN, BUZZER_OFF_LEVEL);
    digitalWrite(RED_LED_PIN, LOW);

    Serial.println();
    Serial.println("POSSIBLE FALL DETECTED - state available to the dashboard when connected.");
    Serial.println("There are 15 seconds to cancel the SOS.");
    Serial.println("Hold touch sensor for 2 seconds to cancel.");
}

void AlarmSystem::startSos() {
    state_ = State::SosActive;
    touchStartedAt_ = 0;
    touchReleased_ = false;
    ledOn_ = true;
    digitalWrite(BUZZER_PIN, BUZZER_ON_LEVEL);
    digitalWrite(RED_LED_PIN, HIGH);
    lastLedToggleAt_ = millis();

    Serial.println();
    Serial.println("EMERGENCY SOS ACTIVE - buzzer on, red LED flashing.");
    Serial.println("Hold touch sensor for 2 seconds to silence the alarm.");
}

void AlarmSystem::cancelFall() {
    reset();
    Serial.println("Fall countdown cancelled; no SOS was triggered.");
}

void AlarmSystem::acknowledgeSos() {
    reset();
    Serial.println("SOS acknowledged; buzzer and red LED are off.");
}

void AlarmSystem::reset() {
    state_ = State::Normal;
    touchStartedAt_ = 0;
    touchReleased_ = false;
    ledOn_ = false;
    digitalWrite(BUZZER_PIN, BUZZER_OFF_LEVEL);
    digitalWrite(RED_LED_PIN, LOW);
}

void AlarmSystem::readSerialCommand() {
    while (Serial.available() > 0) {
        const char command = static_cast<char>(Serial.read());
        if (command == 'f' || command == 'F') {
            startFallCountdown();
        } else if (command == 'r' || command == 'R') {
            reset();
            Serial.println("Alarm reset; dashboard status cleared.");
        }
        // Ignore the Enter key and any other characters from Serial Monitor.
    }
}

void AlarmSystem::readTouchInput(uint32_t now) {
    if (state_ == State::Normal) return;

    const bool touched = digitalRead(TOUCH_PIN) == TOUCH_ACTIVE_LEVEL;
    if (!touched) {
        touchReleased_ = true;
        touchStartedAt_ = 0;
        return;
    }

    // Require the sensor to be released once, then held for two seconds. This
    // avoids cancelling the alarm just because the input starts HIGH at boot.
    if (!touchReleased_) return;
    if (touchStartedAt_ == 0) touchStartedAt_ = now;
    if (now - touchStartedAt_ < TOUCH_HOLD_TIME_MS) return;

    if (state_ == State::FallPending) cancelFall();
    else if (state_ == State::SosActive) acknowledgeSos();
}

void AlarmSystem::updateCountdown(uint32_t now) {
    if (state_ != State::FallPending) return;

    const uint32_t elapsed = now - fallStartedAt_;
    if (elapsed >= FALL_CANCEL_TIME_MS) {
        startSos();
        return;
    }

    // Print a simple once-per-second countdown in Serial Monitor.
    if (lastCountdownAt_ == 0 || now - lastCountdownAt_ >= 1000) {
        lastCountdownAt_ = now;
        const uint32_t secondsLeft = (FALL_CANCEL_TIME_MS - elapsed + 999) / 1000;
        Serial.printf("SOS in %lu seconds\n", static_cast<unsigned long>(secondsLeft));
    }
}

void AlarmSystem::updateOutputs(uint32_t now) {
    if (state_ != State::SosActive) return;

    // The buzzer stays on; the red LED toggles twice per second.
    digitalWrite(BUZZER_PIN, BUZZER_ON_LEVEL);
    if (now - lastLedToggleAt_ >= LED_FLASH_INTERVAL_MS) {
        lastLedToggleAt_ = now;
        ledOn_ = !ledOn_;
        digitalWrite(RED_LED_PIN, ledOn_ ? HIGH : LOW);
    }
}
