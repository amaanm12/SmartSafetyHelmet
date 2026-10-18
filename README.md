# Smart Safety Helmet

ML fall detection is integrated from `ml-algorithm` commit `766dd55`.
`include/fall_model.h` contains the unchanged model; `src/detection.cpp`
contains the helper and its buffer declarations copied verbatim from the
teammate's `main.cpp`, exposed through `include/detection.h`.

`src/main.cpp` samples the MPU6050 on SDA GPIO21 / SCL GPIO22 once per loop
with `delay(10)`, matching the teammate's loop. It feeds all six acceleration
and gyroscope axes into `updateFallModel(...)`. The first prediction needs
100 samples (about 1 second); later predictions run every 20 new samples
(about 200 ms). A probability of at least 0.6 is a positive prediction.

A positive prediction calls `onFallDetected()` to start the existing
15-second cancellable SOS countdown. The Wi-Fi API and dashboard already
report the countdown and SOS state. Every positive prediction calls the alarm,
which ignores calls while a countdown or SOS is already active. A subsequent
positive prediction can start a new countdown after the worker cancels it.
The dashboard's separate high-acceleration indicator remains unchanged.

Build with `pio run`, then upload with `pio run --target upload` (COM3).
The inference cadence and model scores can be checked on a computer with:

```powershell
g++ -std=c++11 -I include src/detection.cpp test/fall_model_window_test.cpp -o .pio/fall_model_window_test.exe
.\.pio\fall_model_window_test.exe
```

These checks validate integration, not real-world detection accuracy.


