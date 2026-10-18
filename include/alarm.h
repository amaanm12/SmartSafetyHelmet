#pragma once

// Fall/SOS alarm with a touch sensor, buzzer and red LED. alarmUpdate() must be
// called often from loop() so touch taps, countdowns and LED flashing stay responsive.
void alarmBegin();
void alarmUpdate();

// Starts the 15 second fall countdown (cancel by tapping the touch sensor).
// Also used by the dashboard's "Trigger SOS" button. Returns false if a
// countdown or SOS is already running.
bool onFallDetected();

// Called by the dashboard's "Call worker" button. Beeps the buzzer until the
// worker taps the touch sensor or the call times out. Returns false if an
// SOS or fall countdown is already using the buzzer.
bool alarmCallWorker();

bool alarmIsSosActive();
bool alarmIsCallActive();

// Seconds left before the fall countdown turns into an SOS, or 0 if no
// countdown is running.
unsigned long alarmSosCountdownSeconds();
