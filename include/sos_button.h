#pragma once

#include <Arduino.h>

// Reads a momentary button wired between the selected GPIO pin and GND.
// update() returns true while the button is pressed and filters quick contact bounce.
class SosButton {
public:
    void begin(int pin);
    bool update();

private:
    int pinNumber = -1;
    bool rawState = false;
    bool stableState = false;
    uint32_t rawChangedAt = 0;
};
