
#include <Arduino.h>



#define TOUCH_PIN 27
#define BUZZER_PIN 25
#define RED_LED_PIN 26

#define TOUCH_ACTIVE HIGH

#define BUZZER_ON LOW
#define BUZZER_OFF HIGH

const unsigned long FALL_CANCEL_TIME = 15000;
const unsigned long TOUCH_HOLD_TIME = 2000;
const unsigned long LED_FLASH_INTERVAL = 500;

enum SystemState {
    NORMAL,
    FALL_PENDING,
    SOS_ACTIVE
};

SystemState state = NORMAL;

unsigned long fallStart = 0;
unsigned long touchStart = 0;
unsigned long lastCountdown = 0;
unsigned long lastLedToggle = 0;

bool touchReleased = false;
bool ledState = false;


void onFallDetected() {

    if (state != NORMAL) return;

    state = FALL_PENDING;

    fallStart = millis();
    touchStart = 0;
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
    Serial.println("Hold touch for 2 seconds");
}


void triggerSOS() {

    state = SOS_ACTIVE;

    touchStart = 0;
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
    Serial.println("Hold touch 2 sec to silence");


}

void cancelPendingSOS() {

    state = NORMAL;
    touchStart = 0;
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
    touchStart = 0;
    touchReleased = false;

    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    digitalWrite(RED_LED_PIN, LOW);
    ledState = false;

    Serial.println();
    Serial.println("SOS ACKNOWLEDGED");
    Serial.println("BUZZER OFF");
    Serial.println("RED LED OFF");


}


void setup() {

    Serial.begin(115200);
    delay(500);

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
    Serial.println("r = Reset System");
}


void loop() {

    unsigned long now = millis();

    // SERIAL COMMANDS
    if (Serial.available()) {

        char command = Serial.read();

        if (command == 'f') {
            onFallDetected();
        }

        if (command == 'r') {
            state = NORMAL;
            touchStart = 0;
            touchReleased = false;
            ledState = false;

            digitalWrite(BUZZER_PIN, BUZZER_OFF);
            digitalWrite(RED_LED_PIN, LOW);

            Serial.println("SYSTEM RESET");
        }
    }

    // READ TOUCH SENSOR
    bool touched =
        digitalRead(TOUCH_PIN) == TOUCH_ACTIVE;

    // TOUCH HOLD DETECTION
    if (state == FALL_PENDING ||
        state == SOS_ACTIVE) {

        if (!touched) {
            touchReleased = true;
            touchStart = 0;
        }

        if (touchReleased && touched) {

            if (touchStart == 0) {
                touchStart = now;
            }

            if (now - touchStart >= TOUCH_HOLD_TIME) {

                if (state == FALL_PENDING) {
                    cancelPendingSOS();
                }
                else if (state == SOS_ACTIVE) {
                    acknowledgeSOS();
                }
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

    delay(10);
}
