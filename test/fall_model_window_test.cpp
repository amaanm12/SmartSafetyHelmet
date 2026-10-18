// Host-side windowing regression test; does not require an attached ESP32.
#include <cassert>
#include <cmath>
#include <cstdio>
#include "detection.h"
#include "fall_model.h"

int main() {
    constexpr size_t count = 420;
    float samples[count][6];
    float score = -1.0f;
    bool fall = false;
    size_t predictions = 0;
    for (size_t n = 0; n < count; ++n) {
        for (size_t c = 0; c < 6; ++c) {
            samples[n][c] = static_cast<float>(
                std::sin(n * 0.07 + c) * (n < 200 ? 0.1 : 12.0));
        }
        samples[n][2] += 9.81f;
        const float oldScore = score;
        const bool oldFall = fall;
        const bool ready = updateFallModel(
            samples[n][0], samples[n][1], samples[n][2],
            samples[n][3], samples[n][4], samples[n][5], score, fall);
        const bool expectedReady = n >= 99 && (n - 99) % 20 == 0;
        assert(ready == expectedReady);
        if (!ready) {
            assert(score == oldScore && fall == oldFall);
            continue;
        }
        ++predictions;
        float chronological[100][6];
        for (size_t i = 0; i < 100; ++i) {
            for (size_t c = 0; c < 6; ++c) {
                chronological[i][c] = samples[n - 99 + i][c];
            }
        }
        const float expected = fall_detector::probability(chronological);
        assert(std::isfinite(score));
        assert(std::fabs(score - expected) < 1e-6f);
        assert(fall == (expected >= fall_detector::THRESHOLD));
    }
    assert(predictions == 17);
    std::puts("PASS: warmup, stride, rolling windows, scores and threshold.");
}
