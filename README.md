# ESP32 Egg Incubator

**John Minton** &lt;cjohnweb@gmail.com&gt;

ESP32-based incubator controller with web interface, real-time sensor monitoring, and automatic temperature and humidity control. Supports multiple modes (Manual, Egg Holding, Incubation), preset bird profiles, and optional egg tilting (stepper stub; hardware to be wired).

## Features

- **DHT22 temperature & humidity** – GPIO 4; readings every 2 s. On read failure, lamp and humidifier stay in last state; UI keeps last-good values.
- **Climate control** – Lamp (GPIO 5) and humidifier (GPIO 6); on = LOW. Mode-agnostic: uses resolved temp/humidity ranges to turn outputs on/off.
- **Modes** – **Manual** (user sets temp/humidity ranges and turn interval), **Egg Holding**, **Incubation** (profile-based phases; Start Incubation from Egg Holding when ready).
- **Profiles / Preload** – Preset profiles (Chicken, Duck, Quail, etc.); “Preload” when Manual. Empty “—” clears presets and disables Start.
- **Egg tilting** – Interval-based tilting with left / flat / right positions; platform returns to flat when tilting is off (lockdown or manual off). Stepper logic is implemented; motor wiring is pending.
- **Web UI** – Dark theme; Mode and Profile/Preload dropdowns; manual targets when Manual; egg tilting toggle; Overview (Day, Start/Stop, Start Incubation when Egg Holding); temperature peaks; communication timer.
- **WebSocket** – Port 81; status every 2 s and on tilt; commands: set_mode, start_process, set_profile, set_manual_targets, set_turning, tilt_now, reset.
- **WiFi** – AP (“Incubator”, default pass 12345678) + STA; credentials and settings (including OTA password) via **Settings** page; NVS persistence; STA MAC kept consistent (DHCP).
- **OTA** – ArduinoOTA; optional password configurable on the Settings page (stored in NVS; applied after save and on next boot).

## Hardware

- ESP32 (or S3) dev board
- DHT22 on **GPIO 4**
- Lamp relay on **GPIO 5** (on = LOW)
- Humidifier relay on **GPIO 6** (on = LOW)
- Alarm outputs (active HIGH): **TEMP_ALARM_PIN** and **HUMIDITY_ALARM_PIN** (configurable in `incubator.ino`)
- Stepper / tilt mechanism: GPIO and wiring TBD (stub in place)

Alarm outputs are asserted when the sensor value is outside the current target range by more than a small margin (defaults: 0.5°F temp, 5% RH humidity).

## Setup

1. **Hardware** – Connect DHT22 to GPIO 4; lamp and humidifier relays to GPIO 5 and 6 (active LOW).
2. **Libraries** – DHT sensor (Adafruit), Adafruit Unified Sensor, WebSockets (Markus Sattler), ArduinoJson. Install via Arduino Library Manager.
3. **Flash** – Open `incubator.ino`, select board, compile and upload.
4. **First run** – Device starts AP “Incubator” (password ≥8 chars, default `12345678`). Connect to it, open **http://10.0.0.1** (or the shown AP IP). Optionally open **Settings** to configure STA WiFi and OTA password, then **Save & Apply**.

## Web interface

- **Main page** – System (WiFi, WebSocket, IPs, MAC), sensor readings (temp °F/°C, humidity, dew, heat index, absolute humidity), lamp/humidifier state, Actions (Mode, Profile/Preload, manual targets when Manual), Egg Tilting (next/last tilt, on/off in Manual), Overview (Day, Start/Stop, Start Incubation when Egg Holding), Temperature Peaks.
- **Settings** (**/wifi**) – WiFi SSID and password, “Keep Access Point enabled”, **OTA password** (optional; leave blank to keep current or disable). Save & Apply stores credentials and OTA password; device reconnects; OTA password applies after next boot (or next OTA attempt if you add runtime apply later).

## API (HTTP)

- `GET /` – Main UI
- `GET /wifi` – Settings page (WiFi + OTA password)
- `GET /api/wifi` – Current WiFi settings and whether OTA password is set (no plaintext password)
- `POST /api/wifi` – Body: `{ "ssid", "pass", "keep_ap", "ota_password" }` (ota_password optional)
- `GET /api/wifi/scan` – List of scanned networks
- `POST /api/reset` – Reboot device
- `GET /ping` – Reachability check

Control (mode, process, profile, manual targets, turning, tilt_now, reset) is via **WebSocket** on port 81, not REST.

## OTA updates

1. Ensure device is on your network (STA) and, if you set one, that OTA password is configured on the Settings page.
2. In Arduino IDE: **Tools → Port →** select “Incubator at &lt;IP&gt;”.
3. Upload over the air. Enter the OTA password if you set one.

## License

Copyright (c) 2025 John Minton

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to use, copy, modify, and distribute the Software for **non-commercial purposes only**, subject to the following conditions:

1. The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
2. **Commercial use is prohibited** without express written permission from the copyright holder.
3. The Software is provided “AS IS”, without warranty of any kind.

**Commercial use inquiries:** cjohnweb@gmail.com

## Contributing

Contributions are welcome via pull requests or issues for bugs and feature requests.
