#include <Arduino.h>
#include <WiFi.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("ESP32-WROOM-32 is working!");

    WiFi.mode(WIFI_STA);
    Serial.println("Scanning WiFi networks...");
}

void loop() {
    int networks = WiFi.scanNetworks();

    Serial.printf("Found %d networks\n", networks);

    for (int i = 0; i < networks; i++) {
        Serial.println(WiFi.SSID(i));
    }

    delay(5000);
}