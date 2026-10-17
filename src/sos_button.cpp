#include "sos_button.h"

namespace {
constexpr uint32_t DEBOUNCE_INTERVAL_MS = 35;
}

void SosButton::begin(int pin) {
    pinNumber = pin;
    // The ESP32's internal pull-up keeps the input HIGH while the button is
    // released. Pressing the button connects the pin to GND, making it LOW.
    pinMode(pinNumber, INPUT_PULLUP);
    rawState = digitalRead(pinNumber) == LOW;
    stableState = rawState;
    rawChangedAt = millis();
}

bool SosButton::update() {
    if (pinNumber < 0) return false;

    const uint32_t now = millis();
    const bool pressed = digitalRead(pinNumber) == LOW;
    if (pressed != rawState) {
        rawState = pressed;
        rawChangedAt = now;
    }
    // Physical buttons can rapidly flicker between HIGH and LOW for a few
    // milliseconds. Accept the new state only after it stays steady.
    if (now - rawChangedAt >= DEBOUNCE_INTERVAL_MS) {
        stableState = rawState;
    }
    return stableState;
}
