#include "wifi_communication.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <Ticker.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <string.h>

namespace {
// The browser requests the worker API over normal HTTP (port 80). The UDP
// beacon is sent separately so local network tools can spot this ESP32.
constexpr uint16_t HTTP_PORT = 80;
constexpr uint16_t DISCOVERY_PORT = 4210;
constexpr uint32_t BEACON_INTERVAL_MS = 2000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
constexpr char MDNS_HOST[] = "shieldguard";

// The red alarm LED (RED_LED_PIN in alarm.cpp) doubles as a Wi-Fi status light
// during startup, so the state is visible without a serial monitor:
//   fast blink = connecting to saved Wi-Fi
//   slow blink = setup Wi-Fi (ShieldGuard-Setup-...) is open, join it with a phone
//   solid for 2 seconds then off = connected
constexpr int STATUS_LED_PIN = 26;
constexpr float CONNECTING_BLINK_S = 0.1f;
constexpr float SETUP_PORTAL_BLINK_S = 1.0f;

Ticker statusBlinker;

WebServer httpServer(HTTP_PORT);
WiFiUDP discoveryUdp;
IMUData currentSensorData{};
AlarmStatus currentAlarm{};
AlarmCommandHandler callWorkerHandler = nullptr;
AlarmCommandHandler triggerSosHandler = nullptr;
uint32_t lastBeaconAt = 0;
uint32_t lastWifiRetryAt = 0;

void addCorsHeaders() {
    // GitHub Pages and the ESP32 have different web addresses (origins). These
    // headers let the browser page read this device's response. They do not
    // bypass the browser's separate local-network permission prompt.
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpServer.sendHeader("Access-Control-Allow-Headers", "Content-Type, Accept");
    httpServer.sendHeader("Access-Control-Allow-Private-Network", "true");
    httpServer.sendHeader("Cache-Control", "no-store, max-age=0");
}

void handlePreflight() {
    addCorsHeaders();
    httpServer.send(204);
}

void handleHealth() {
    addCorsHeaders();
    httpServer.send(200, "application/json",
                    "{\"service\":\"shieldguard\",\"connected\":true,\"workerCount\":1}");
}

void handleWorkerData() {
    char body[512];
    // Turn the latest sensor sample into JSON, the text format the dashboard
    // understands. temperatureC is null because the MPU6050 measures its own
    // chip temperature, not the worker's body temperature.
    const int length = snprintf(
        body,
        sizeof(body),
        "{\"workers\":[{\"id\":\"1\",\"workerId\":\"1\",\"name\":\"Worker #1\",\"active\":true,\"temperatureC\":null,\"sensorTemperatureC\":%.2f,\"acceleration\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},\"gyroscope\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},\"sos\":%s,\"calling\":%s,\"sosCountdownS\":%lu,\"updatedAtMs\":%lu}],\"ip\":\"%s\"}",
        currentSensorData.temp,
        currentSensorData.accX, currentSensorData.accY, currentSensorData.accZ,
        currentSensorData.gyroX, currentSensorData.gyroY, currentSensorData.gyroZ,
        currentAlarm.sosActive ? "true" : "false",
        currentAlarm.callActive ? "true" : "false",
        currentAlarm.sosCountdownS,
        static_cast<unsigned long>(millis()),
        // The dashboard switches to this IP after connecting, so it keeps
        // working even when shieldguard.local (mDNS) stops resolving.
        WiFi.localIP().toString().c_str());

    addCorsHeaders();
    if (length <= 0 || length >= static_cast<int>(sizeof(body))) {
        httpServer.send(500, "application/json",
                        "{\"error\":\"response serialization failed\"}");
        return;
    }
    httpServer.send(200, "application/json", body);
}

void handleCallWorker() {
    // The dashboard's "Call worker" button sends POST /api/call. The alarm
    // decides whether it can buzz (it refuses during a fall countdown or SOS).
    const bool started = callWorkerHandler != nullptr && callWorkerHandler();
    if (started) currentAlarm.callActive = true;

    addCorsHeaders();
    if (started) {
        httpServer.send(200, "application/json", "{\"calling\":true}");
    } else {
        httpServer.send(409, "application/json",
                        "{\"calling\":false,\"error\":\"helmet alarm is busy\"}");
    }
}

void handleTriggerSos() {
    // The dashboard's "Trigger SOS" button sends POST /api/sos. It starts the
    // same 15 second fall countdown as a detected fall, so the worker can still
    // cancel it with a tap before the SOS goes off.
    const bool started = triggerSosHandler != nullptr && triggerSosHandler();

    addCorsHeaders();
    if (started) {
        httpServer.send(200, "application/json", "{\"countdownStarted\":true}");
    } else {
        httpServer.send(409, "application/json",
                        "{\"countdownStarted\":false,\"error\":\"countdown or SOS already running\"}");
    }
}

void startHttpService() {
    // Each route is a small URL handled by the ESP32:
    // GET /api/data returns readings; GET /api/health is a simple connection check;
    // POST /api/call makes the helmet buzzer beep; POST /api/sos starts the
    // 15 second SOS countdown.
    httpServer.on("/api/data", HTTP_GET, handleWorkerData);
    httpServer.on("/api/data", HTTP_OPTIONS, handlePreflight);
    httpServer.on("/api/health", HTTP_GET, handleHealth);
    httpServer.on("/api/health", HTTP_OPTIONS, handlePreflight);
    httpServer.on("/api/call", HTTP_POST, handleCallWorker);
    httpServer.on("/api/call", HTTP_OPTIONS, handlePreflight);
    httpServer.on("/api/sos", HTTP_POST, handleTriggerSos);
    httpServer.on("/api/sos", HTTP_OPTIONS, handlePreflight);
    httpServer.onNotFound([]() {
        addCorsHeaders();
        httpServer.send(404, "application/json", "{\"error\":\"not found\"}");
    });
    httpServer.begin();
    Serial.printf("HTTP API: http://%s.local/api/data and http://%s/api/data\n",
                  MDNS_HOST, WiFi.localIP().toString().c_str());
}

void sendDiscoveryBeacon() {
    const IPAddress ip = WiFi.localIP();
    const IPAddress subnetMask = WiFi.subnetMask();
    IPAddress broadcastIp = ip;
    for (uint8_t octet = 0; octet < 4; ++octet) {
        const uint8_t hostMask = static_cast<uint8_t>(~subnetMask[octet]);
        broadcastIp[octet] = static_cast<uint8_t>((ip[octet] & subnetMask[octet]) | hostMask);
    }

    char beacon[180];
    snprintf(beacon, sizeof(beacon),
             "{\"service\":\"shieldguard\",\"host\":\"%s.local\",\"ip\":\"%u.%u.%u.%u\",\"port\":%u,\"workerId\":\"1\"}",
             MDNS_HOST, ip[0], ip[1], ip[2], ip[3], HTTP_PORT);

    // Announce the device to other programs on this Wi-Fi subnet. Browsers
    // cannot listen for UDP directly, so the dashboard finds this node by mDNS.
    discoveryUdp.beginPacket(broadcastIp, DISCOVERY_PORT);
    discoveryUdp.write(reinterpret_cast<const uint8_t *>(beacon), strlen(beacon));
    discoveryUdp.endPacket();
}

void toggleStatusLed() {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
}

void showConnectedOnLed() {
    // Solid on is clearly different from either blink pattern.
    statusBlinker.detach();
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(2000);
    digitalWrite(STATUS_LED_PIN, LOW);
}

void connectToWifi() {
    // Ticker runs the blink in the background while autoConnect() blocks.
    pinMode(STATUS_LED_PIN, OUTPUT);
    statusBlinker.attach(CONNECTING_BLINK_S, toggleStatusLed);

    // Station mode means this ESP32 joins an existing router or phone hotspot.
    // WiFiManager remembers the selected network, so credentials do not need
    // to be typed into the source code.
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(MDNS_HOST);
    WiFi.setAutoReconnect(true);

    const String macAddress = WiFi.macAddress();
    const String suffix = macAddress.substring(9, 17);
    const String setupSsid = "ShieldGuard-Setup-" + suffix;
    const String setupPassword = "SG-" + suffix + "-Pair";

    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(300);
    wifiManager.setAPCallback([](WiFiManager *) {
        statusBlinker.attach(SETUP_PORTAL_BLINK_S, toggleStatusLed);
    });
    // If no saved network is available, WiFiManager creates this temporary
    // password-protected setup network. The serial monitor shows its details.
    Serial.println("Connecting to saved Wi-Fi. If needed, setup Wi-Fi opens for 5 minutes.");
    Serial.printf("Setup Wi-Fi: %s\nSetup password: %s\n", setupSsid.c_str(), setupPassword.c_str());
    if (!wifiManager.autoConnect(setupSsid.c_str(), setupPassword.c_str())) {
        Serial.println("Wi-Fi setup timed out; restarting to try again.");
        delay(1000);
        ESP.restart();
    }

    showConnectedOnLed();
    Serial.printf("Connected to Wi-Fi. ESP32 IP: %s\n", WiFi.localIP().toString().c_str());
}
}  // namespace

void WifiCommunication::begin(AlarmCommandHandler onCallWorker, AlarmCommandHandler onTriggerSos) {
    callWorkerHandler = onCallWorker;
    triggerSosHandler = onTriggerSos;
    connectToWifi();

    // mDNS gives the ESP32 a readable local name. On the same Wi-Fi, the
    // dashboard can try shieldguard.local instead of needing a typed IP address.
    if (MDNS.begin(MDNS_HOST)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        MDNS.addServiceTxt("http", "tcp", "path", "/api/data");
        Serial.printf("mDNS discovery: http://%s.local/api/data\n", MDNS_HOST);
    } else {
        Serial.println("mDNS startup failed; use the printed ESP32 IP address.");
    }

    discoveryUdp.begin(DISCOVERY_PORT + 1);
    startHttpService();
}

void WifiCommunication::update(const IMUData& sensorData, const AlarmStatus& alarmStatus) {
    // Copy the newest sample so an HTTP request always returns current readings.
    currentSensorData = sensorData;
    currentAlarm = alarmStatus;

    const uint32_t now = millis();
    if (WiFi.status() == WL_CONNECTED) {
        // Check for a browser request and answer it when one is waiting.
        httpServer.handleClient();
        if (now - lastBeaconAt >= BEACON_INTERVAL_MS) {
            lastBeaconAt = now;
            sendDiscoveryBeacon();
        }
    } else if (now - lastWifiRetryAt >= WIFI_RETRY_INTERVAL_MS) {
        // A temporary router disconnect should not require a manual ESP32 reset.
        lastWifiRetryAt = now;
        Serial.println("Wi-Fi disconnected; retrying...");
        WiFi.reconnect();
    }
}
