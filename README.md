# Smart Safety Helmet

## How the ESP32 and dashboard work together

There are two separate programs:

1. The ESP32 firmware reads the MPU6050 and SOS button. It joins your Wi-Fi
   network and makes the latest reading available from a small web API.
2. The dashboard is the `index.html` page. Your browser opens that page and
   asks the ESP32 for the latest readings every two seconds.

The dashboard does not connect to the MPU6050 directly. The path is:

```text
MPU6050 and SOS button → ESP32 → Wi-Fi router → browser running the dashboard
```

The browser's repeated HTTP requests are sometimes called polling. The ESP32
does not push data into GitHub Pages; the browser checks the ESP32 for fresh
data each time. If a node that was working stops answering, the page shows a
red **WORKER DISCONNECTED** message and keeps retrying.

## First-time Wi-Fi setup

You do not need to put your Wi-Fi password in the source code. The firmware
uses WiFiManager to save the network details on the ESP32:

1. Connect the ESP32 to your computer and open the PlatformIO Serial Monitor at
   **115200 baud**.
2. Power on or reset the ESP32. If it has no saved Wi-Fi network, the serial
   output prints a temporary network name such as `ShieldGuard-Setup-...` and
   its password.
3. On a phone or computer, join that temporary setup network. A setup page
   should open. If it does not, browse to `http://192.168.4.1` while connected
   to the setup network.
4. Choose the Wi-Fi network the ESP32 should join and enter that network's
   password. The ESP32 saves these details and joins the network.
5. The serial output prints the ESP32's local IP address and the dashboard URL:
   `http://shieldguard.local/api/data`.
6. Connect the computer or phone that will show the dashboard to that same
   Wi-Fi network. Open the GitHub Pages dashboard and choose **Search active
   worker** if it has not connected automatically.

The temporary `ShieldGuard-Setup-*` network is only for configuring Wi-Fi. It
is not the normal connection used to send worker readings.

## Do the ESP32 and dashboard need the same Wi-Fi?

For automatic discovery, yes: keep them on the same local Wi-Fi network. A
phone hotspot can work if both the ESP32 and a separate dashboard device join
that hotspot and the hotspot allows devices to talk to each other. Guest Wi-Fi
and some hotspots isolate connected devices, which blocks discovery and data.

The ESP32 sends a UDP discovery announcement on port `4210`. Browsers cannot
listen for UDP packets, so the dashboard finds this one-worker setup with the
local name `shieldguard.local` over HTTP. The UDP announcement is available to
local network tools and future bridge software. If mDNS name lookup does not
work on your network, enter the local IP printed in Serial Monitor using the
dashboard's **Edit IP** button.

GitHub Pages uses HTTPS, while this ESP32 API uses HTTP. When the browser asks,
allow the dashboard site to access your local network. If the request is still
blocked, the Connection Settings panel explains the browser's Insecure Content
setting. The ESP32 API includes CORS headers so the browser can read its
response. CORS does not grant local-network access by itself.

## What the dashboard receives

Every two seconds the browser requests `GET /api/data`. The ESP32 responds with
JSON, a structured text format. For example:

```json
{
  "workers": [
    {
      "id": "1",
      "name": "Worker #1",
      "active": true,
      "temperatureC": null,
      "sensorTemperatureC": 25.4,
      "acceleration": { "x": 0.1, "y": 0.0, "z": 9.8 },
      "gyroscope": { "x": 0.0, "y": 0.1, "z": 0.0 },
      "sos": false
    }
  ]
}
```

The acceleration values are in metres per second squared (`m/s²`). Gyroscope
values are in radians per second (`rad/s`). `sos` becomes `true` while the
optional button is pressed.

The MPU6050 has a small temperature sensor inside the chip. It measures the
chip temperature, not a worker's body temperature. That is why the ESP32 sends
`temperatureC: null` for body temperature and sends the chip reading separately
as `sensorTemperatureC`. The dashboard's body-temperature average will show no
reading until a body-temperature sensor is added.

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
| SOS switch | GPIO 27 to GND | Optional momentary pushbutton; pressed means SOS active. |

```text
ESP32-WROOM-32             MPU6050
3V3 ---------------------- VCC
GND ---------------------- GND
GPIO 21 ------------------ SDA
GPIO 22 ------------------ SCL
GND ---------------------- AD0 (optional; default address 0x68)
GPIO 27 ---- momentary switch ---- GND (optional SOS button)
```

The firmware initializes I2C with GPIO 21 for SDA and GPIO 22 for SCL. If you
change those wires, update `I2C_SDA_PIN` or `I2C_SCL_PIN` in `src/main.cpp`.

The optional SOS switch connects GPIO 27 to GND when pressed. The firmware uses
the ESP32's internal pull-up resistor, so no external resistor is needed.

## Where the code lives

- `src/main.cpp` starts each helper and passes sensor readings to the network.
- `src/sensor.cpp` reads acceleration, gyroscope, and MPU6050 chip temperature.
- `src/wifi_communication.cpp` connects Wi-Fi, serves the HTTP API, publishes
  the mDNS name, and sends UDP discovery announcements.
- `src/sos_button.cpp` reads the SOS switch and filters button contact bounce.
- `index.html` contains the dashboard, its automatic search, and demo fallback.

## If the dashboard cannot find the worker

- Check Serial Monitor at 115200 baud for `Connected to Wi-Fi` and the IP address.
- Check that both devices joined the same Wi-Fi network and that it does not
  isolate connected devices.
- Try the IP address from Serial Monitor in the dashboard's **Edit IP** panel.
- Allow the browser's local-network prompt for the GitHub Pages site. If needed,
  follow the Insecure Content instructions in Connection Settings.
- Open `http://<ESP32-IP>/api/health` on the same network. A JSON response means
  the ESP32 web server is reachable; a browser error may still mean the
  dashboard's cross-origin or local-network permission was blocked.
- If the page says **WORKER DISCONNECTED**, check ESP32 power and Wi-Fi signal.
