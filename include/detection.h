#pragma once

// Feed one fresh sample (acceleration in m/s^2, gyroscope in rad/s), nominally
// every 10 ms, matching the ml-algorithm branch. Returns true only when a new
// prediction is available: after 100 samples, then every 20 new samples.
// score and fallEvent are only written when the return value is true.
bool updateFallModel(float ax, float ay, float az,
                     float gx, float gy, float gz,
                     float& score, bool& fallEvent);
