# Aadhar Pumper Firmware

ESP32 firmware for **Aadhar**, a device that stands in potholes and other spots where rain water collects. When the water goes still, it runs a pump to ripple the surface so mosquito larvae can't develop.

## How it works

Every check (default 3 h) the three water probes are read. If any probe shows water at a level that hasn't changed since the last check, the pump runs (default 15 min). If all probes are full, the pump runs at most once every 6 h. Thresholds and timings can be changed from the dashboard.

## Hardware

| Part | ESP32 pin |
|---|---|
| Water level sensor, top (resistive, HW-038 type) | GPIO 32 |
| Water level sensor, middle | GPIO 33 |
| Water level sensor, bottom | GPIO 34 |
| 1-channel relay module (active HIGH), switches the pump | GPIO 27 |

Board: ESP32 DevKit (ESP32-WROOM-32). Probes use ADC1 pins only, because ADC2 doesn't work while WiFi is on. If your relay module is active LOW, set `RELAY_ON` to `LOW` in `Aadhar/Aadhar.ino`.

## Setup

1. Install [PlatformIO](https://platformio.org/).
2. Copy `Aadhar/secrets.example.h` to `Aadhar/secrets.h` and fill in your WiFi name, WiFi password and a hotspot password (8+ characters).
3. Set `upload_port` / `monitor_port` in `platformio.ini` to your board's port.
4. Build and upload:
   ```
   pio run -t upload
   ```
   If the board doesn't enter download mode by itself: unplug USB, hold BOOT, plug USB back in, release BOOT.

## Dashboard

The ESP32 always runs its own WiFi hotspot named **Aadhar**. Join it and open **http://192.168.4.1**. No router is needed.

If the WiFi network from `secrets.h` is in range, the ESP32 joins it too, and the dashboard is also at **http://aadhar.local** (or the IP printed on the serial monitor at 115200 baud). The saved WiFi can be changed from the dashboard.

The dashboard shows live probe readings, a 1-hour chart and an event log. From it you can start or stop the pump, run a check and change settings.
