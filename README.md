# Smart Safety Helmet

This project has two parts: ESP32 firmware in `src/` and the manager dashboard in `index.html`.

## Set Wi-Fi using .env

Open `.env` in the project folder and enter the same 2.4 GHz Wi-Fi network
your laptop uses:

```dotenv
WIFI_SSID="Your network name"
WIFI_PASSWORD="Your network password"
```

Save the file, then build and upload again. PlatformIO reads `.env` during the
build and includes its settings in the firmware. The ESP32 tries that network
for 20 seconds at startup, then falls back to saved Wi-Fi or the setup portal.
Leave the values empty to use the original setup portal flow. Keep quotes
around values containing spaces or special characters; their contents are
literal. `.env` and the generated header are ignored by Git. Credentials are
still part of the uploaded firmware, so keep firmware binaries private too.

## Try the serial fall alarm

The firmware accepts a simulated fall command from PlatformIO Serial Monitor. A fall appears on the website right away as **FALL PENDING**. If it is not cancelled within 15 seconds, the ESP32 turns on the buzzer and flashes the red LED; the website changes to **SOS ACTIVE**.

1. Connect the ESP32 to the same Wi-Fi network as the computer or phone that will open the dashboard. On first boot, WiFiManager guides you through joining the Wi-Fi.
2. Upload the project for the `esp32dev` board and open Serial Monitor at **115200 baud**.
3. Open the dashboard and connect it to the ESP32. The ESP32 IP address is printed in Serial Monitor; the page also tries `shieldguard.local`.
4. Type `f` and press Enter. The dashboard should show the pending fall, then SOS after 15 seconds.
5. Type `r` and press Enter to turn the alarm off and clear its dashboard state. The touch sensor can also cancel the countdown or silence SOS if held for two seconds.

Serial Monitor now shows your typing (`monitor_echo = yes`) and sends it when
you press Enter (`send_on_enter`). Click inside the **Terminal** panel running
PlatformIO Serial Monitor; the **Output** panel is a log viewer and cannot send
commands. The alarm commands work while Wi-Fi is still connecting. Once the
network connects, the API starts and the dashboard sees the current alarm.
Reopen Serial Monitor after changing its settings. If the ESP32 keeps printing
only boot/reset lines, it is restarting and cannot process commands normally;
the `[BOOT]` and `[READY]` messages help identify how far startup gets.

The browser polls the ESP32's `http://<ESP32-IP>/api/data` endpoint every two seconds. Because GitHub Pages is HTTPS and the ESP32 API uses local HTTP, allow local-network access for the site. The dashboard's Connection Settings panel has browser-specific guidance.

## Alarm wiring (ESP32-WROOM-32)

| Alarm part | ESP32 pin | Notes |
| --- | --- | --- |
| Touch sensor digital output | GPIO 27 | Active HIGH. The ESP32 uses an internal pull-down. |
| Active-low buzzer module input | GPIO 25 | The code turns the buzzer on by driving LOW. |
| Red LED | GPIO 26 | Add a current-limiting resistor, such as 220 Ω, in series. |
| Grounds | GND | Sensor and ESP32 grounds must be connected together. |

Use a buzzer module or a suitable transistor driver if the buzzer needs more current than an ESP32 GPIO can provide. Do not connect a high-current buzzer directly to the pin.

The MPU6050 remains optional for this alarm demo. Its I2C pins are GPIO 21 (SDA) and GPIO 22 (SCL), so they do not overlap the alarm pins. If the MPU6050 is missing, the firmware continues and the website shows dashes for IMU readings. The previous SOS button reader on GPIO 27 is not started because that pin is used by Amaan's touch sensor.

## Where the code lives

- `src/main.cpp` starts the alarm and Wi-Fi modules and optionally reads the MPU6050.
- `src/alarm.cpp` handles Serial Monitor commands, the 15-second cancel period, touch cancellation, buzzer output, and LED flashing.
- `src/wifi_communication.cpp` connects to Wi-Fi and serves the worker JSON, including `fallDetected`, `sos`, and `alarmState`.
- `index.html` polls the ESP32 and displays pending fall and active SOS states on the worker card.
- `src/sensor.cpp` reads MPU6050 acceleration, rotation, and chip temperature when that sensor is connected.
