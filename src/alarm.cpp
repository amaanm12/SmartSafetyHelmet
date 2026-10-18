
#include <Arduino.h>

#include "alarm.h"



#define TOUCH_PIN 27
#define BUZZER_PIN 25
#define RED_LED_PIN 26

#define TOUCH_ACTIVE HIGH

#define BUZZER_ON LOW
#define BUZZER_OFF HIGH

const unsigned long FALL_CANCEL_TIME = 15000;
const unsigned long LED_FLASH_INTERVAL = 500;

// "Call worker" from the dashboard: beep on/off until answered or timed out.
const unsigned long CALL_BEEP_INTERVAL = 300;
const unsigned long CALL_TIMEOUT = 30000;

enum SystemState {
    NORMAL,
    FALL_PENDING,
    SOS_ACTIVE,
    CALL_ACTIVE
};

SystemState state = NORMAL;

unsigned long fallStart = 0;
unsigned long lastCountdown = 0;
unsigned long lastLedToggle = 0;
unsigned long callStart = 0;
unsigned long lastBeepToggle = 0;

bool touchReleased = false;
bool ledState = false;
bool beepState = false;


bool onFallDetected() {

    // A fall is more important than a supervisor call, so it replaces it.
    if (state != NORMAL && state != CALL_ACTIVE) return false;

    state = FALL_PENDING;

    fallStart = millis();
    lastCountdown = 0;
    touchReleased = false;

    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    digitalWrite(RED_LED_PIN, LOW);
    ledState = false;

    Serial.println();
    Serial.println("========================");
    Serial.println("POSSIBLE FALL DETECTED!");
    Serial.println("========================");
    Serial.println("15 seconds to cancel SOS");
    Serial.println("Tap touch sensor to cancel");

    return true;
}


void triggerSOS() {

    state = SOS_ACTIVE;

    touchReleased = false;


    digitalWrite(BUZZER_PIN, BUZZER_ON);


    ledState = true;
    digitalWrite(RED_LED_PIN, HIGH);
    lastLedToggle = millis();

    Serial.println();
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("EMERGENCY SOS TRIGGERED!");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("BUZZER: ON");
    Serial.println("RED LED: FLASHING");
    Serial.println("Tap touch sensor to silence");


}

void cancelPendingSOS() {

    state = NORMAL;
    touchReleased = false;

    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    digitalWrite(RED_LED_PIN, LOW);
    ledState = false;

    Serial.println();
    Serial.println("SOS CANCELLED");
    Serial.println("No emergency alert triggered");
}

void acknowledgeSOS() {

    state = NORMAL;
    touchReleased = false;

    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    digitalWrite(RED_LED_PIN, LOW);
    ledState = false;

    Serial.println();
    Serial.println("SOS ACKNOWLEDGED");
    Serial.println("BUZZER OFF");
    Serial.println("RED LED OFF");


}

bool alarmCallWorker() {

    // Don't interrupt a fall countdown or an active SOS.
    if (state == FALL_PENDING || state == SOS_ACTIVE) return false;

    state = CALL_ACTIVE;

    callStart = millis();
    lastBeepToggle = callStart;
    touchReleased = false;

    beepState = true;
    digitalWrite(BUZZER_PIN, BUZZER_ON);

    Serial.println();
    Serial.println("========================");
    Serial.println("SUPERVISOR IS CALLING!");
    Serial.println("========================");
    Serial.println("Tap touch sensor to answer");

    return true;
}

void endCall(const char* reason) {

    state = NORMAL;
    touchReleased = false;

    beepState = false;
    digitalWrite(BUZZER_PIN, BUZZER_OFF);

    Serial.println();
    Serial.println(reason);
}

bool alarmIsSosActive() {
    return state == SOS_ACTIVE;
}

bool alarmIsCallActive() {
    return state == CALL_ACTIVE;
}

unsigned long alarmSosCountdownSeconds() {

    if (state != FALL_PENDING) return 0;

    unsigned long elapsed = millis() - fallStart;
    if (elapsed >= FALL_CANCEL_TIME) return 0;

    return (FALL_CANCEL_TIME - elapsed + 999) / 1000;
}


void alarmBegin() {

    pinMode(TOUCH_PIN, INPUT);


    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(RED_LED_PIN, LOW);
    pinMode(RED_LED_PIN, OUTPUT);

    Serial.println();
    Serial.println("========================");
    Serial.println("SMART SAFETY HELMET");
    Serial.println("ESP32-WROOM-32");
    Serial.println("========================");
    Serial.println("System Ready!");
    Serial.println("f = Simulate Fall");
    Serial.println("c = Simulate Call Worker");
    Serial.println("r = Reset System");
}


void alarmUpdate() {

    // SERIAL COMMANDS (handled before reading millis() so a call or fall
    // started here never has a start time later than "now")
    if (Serial.available()) {

        char command = Serial.read();

        if (command == 'f') {
            onFallDetected();
        }

        if (command == 'c') {
            alarmCallWorker();
        }

        if (command == 'r') {
            state = NORMAL;
            touchReleased = false;
            ledState = false;
            beepState = false;

            digitalWrite(BUZZER_PIN, BUZZER_OFF);
            digitalWrite(RED_LED_PIN, LOW);

            Serial.println("SYSTEM RESET");
        }
    }

    unsigned long now = millis();

    // READ TOUCH SENSOR
    bool touched =
        digitalRead(TOUCH_PIN) == TOUCH_ACTIVE;

    // TOUCH TAP DETECTION
    // A tap counts the moment the sensor goes from released to touched. The
    // sensor must be released first, so a finger already resting on it when
    // the alarm starts does not instantly silence it.
    if (state == FALL_PENDING ||
        state == SOS_ACTIVE ||
        state == CALL_ACTIVE) {

        if (!touched) {
            touchReleased = true;
        }
        else if (touchReleased) {

            if (state == FALL_PENDING) {
                cancelPendingSOS();
            }
            else if (state == SOS_ACTIVE) {
                acknowledgeSOS();
            }
            else if (state == CALL_ACTIVE) {
                endCall("CALL ANSWERED");
            }
        }
    }

    // FALL COUNTDOWN
    if (state == FALL_PENDING) {

        unsigned long elapsed = now - fallStart;

        if (elapsed >= FALL_CANCEL_TIME) {
            triggerSOS();
        }
        else if (lastCountdown == 0 ||
                 now - lastCountdown >= 1000) {

            lastCountdown = now;

            unsigned long remaining =
                (FALL_CANCEL_TIME - elapsed + 999) / 1000;

            Serial.print("SOS in ");
            Serial.print(remaining);
            Serial.println(" seconds");
        }
    }

    // SOS ACTIVE
    if (state == SOS_ACTIVE) {

        digitalWrite(BUZZER_PIN, BUZZER_ON);

        if (now - lastLedToggle >=
            LED_FLASH_INTERVAL) {

            lastLedToggle = now;

            ledState = !ledState;

            digitalWrite(
                RED_LED_PIN,
                ledState ? HIGH : LOW
            );
        }
    }

    // CALL ACTIVE: beeping buzzer so it sounds different from the SOS tone
    if (state == CALL_ACTIVE) {

        if (now - callStart >= CALL_TIMEOUT) {
            Serial.printf("\nCall rang for %lu ms\n", now - callStart);
            endCall("CALL NOT ANSWERED (TIMED OUT)");
        }
        else if (now - lastBeepToggle >= CALL_BEEP_INTERVAL) {

            lastBeepToggle = now;

            beepState = !beepState;

            digitalWrite(
                BUZZER_PIN,
                beepState ? BUZZER_ON : BUZZER_OFF
            );
        }
    }
}
