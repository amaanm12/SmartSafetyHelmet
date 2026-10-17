#include <Arduino.h>

#include "sensor.h"
#include "sos_button.h"
#include "wifi_communication.h"

namespace {
// These pins match the ESP32-WROOM-32 wiring in README.md.
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr int SOS_BUTTON_PIN = 27;

IMUSensor imu;
SosButton sosButton;
WifiCommunication wifi;
}

void setup() {
    Serial.begin(115200);

    // setup() runs once after the ESP32 powers on or resets. Stop here if the
    // MPU6050 is not connected, because there would be no real sensor data to send.
    if (!imu.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
        Serial.println("MPU6050 initialization failed. Check the wiring.");
        while (true) delay(1000);
    }

    // Start the button reader and connect the Wi-Fi/API services.
    sosButton.begin(SOS_BUTTON_PIN);
    wifi.begin();
}

void loop() {
    // loop() repeats continuously: read the sensors, pass the latest readings
    // to the web server, then wait briefly before taking the next sample.
    imu.update();
    wifi.update(imu.getData(), sosButton.update());
    delay(100);
}
