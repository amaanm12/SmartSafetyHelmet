# Work Guard

A hard hat that doesn’t just protect—it detects danger, sends alerts, and helps save lives.

## How to Set Up

### 1. Clone the Repository

```bash
git clone -b submission https://github.com/amaanm12/SmartSafetyHelmet.git
```

### 2. Open the Project

1. Install the **PlatformIO IDE** extension in **Visual Studio Code**.
2. Select **File → Open Folder**.
3. Choose the cloned `SmartSafetyHelmet` folder.

### 3. Configure Wi-Fi

Create a `.env` file in the project’s root folder and enter your Wi-Fi credentials:

```env
WIFI_SSID="Your Wi-Fi name"
WIFI_PASSWORD="Your Wi-Fi password"
```

The ESP32 and your computer must connect to the **same Wi-Fi network**.

### 4. Upload the Firmware

1. Connect the ESP32 to your computer using a USB cable.
2. Open the **PlatformIO** side panel and click **Upload**.
3. Hold the ESP32’s **BOOT** button while the uploader connects, then release it once writing begins.
4. Wait for the upload to complete successfully.

### 5. Complete the First-Time Setup

1. Open **Serial Monitor**.
2. Click **Start Monitoring** twice for the initial setup.
3. Check the serial output to confirm the ESP32 connects to Wi-Fi.

### 6. Open the Dashboard

Open `index.html` in your browser to view the dashboard.

Once connected, the dashboard displays live sensor readings and alerts from the ESP32.

## Battery-Powered Operation

After flashing and completing the initial setup, disconnect the USB cable and power the ESP32 with a suitable battery supply.

The ESP32 communicates with the dashboard over Wi-Fi, so a USB connection is no longer required. Keep the ESP32 and your computer on the **same Wi-Fi network**.
