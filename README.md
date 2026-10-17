# Smart Safety Helmet

## ESP32-WROOM-32 to MPU6050 wiring

Connect the MPU6050 breakout to the ESP32 I2C pins used by `src/main.cpp`:

| MPU6050 pin | ESP32-WROOM-32 pin | Notes |
| --- | --- | --- |
| VCC | 3V3 | Use 3.3 V for the sensor breakout. |
| GND | GND | Grounds must be common. |
| SDA | GPIO 21 | I2C data. |
| SCL | GPIO 22 | I2C clock. |
| AD0 | GND (optional) | Selects the default I2C address, usually `0x68`. |
| INT | Not connected | Not used by this firmware. |

```text
ESP32-WROOM-32             MPU6050
3V3 ---------------------- VCC
GND ---------------------- GND
GPIO 21 ------------------ SDA
GPIO 22 ------------------ SCL
GND ---------------------- AD0 (optional; default address 0x68)
```

The firmware initializes the sensor with `Wire.begin(21, 22)`. If you use
different pins, change the arguments in `imu.begin(sdaPin, sclPin)` in
`src/main.cpp`.
