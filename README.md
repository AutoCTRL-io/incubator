# ESP32 Chicken Egg Incubator
### John Minton <cjohnweb@gmail.com>

An ESP32-S3 based incubator controller with web interface, real-time sensor monitoring, and automatic temperature control. **Supports 37 preconfigured bird species with optimized temperature profiles, plus a Custom mode for any other species or specific temperature requirements.**

## Features

- **DHT22 Temperature & Humidity Sensor** - Real-time monitoring
- **Automatic Temperature Control** - Relay-based heating control
- **Web Interface** - Modern dark-themed UI accessible via browser
- **WebSocket Real-time Updates** - Live sensor data streaming
- **WiFi Configuration** - Easy setup via web interface
- **OTA Updates** - Over-the-air firmware updates
- **Multiple Egg Profiles** - Pre-configured settings for 38 bird species (alphabetically organized from Chicken to Vulture) plus Custom mode for any species
- **Temperature Peak Tracking** - Records highest temperature peaks for multiple time windows (6h, 3h, 1.5h, 1h, 30m, 15m)
- **Alarm System** - GPIO-based alarms for temperature and humidity out-of-range conditions
- **Dual WiFi Mode** - Access Point + Station mode for always-accessible setup

## Hardware Requirements

- ESP32 or S3 development board
- DHT22 temperature/humidity sensor
- Relay module (for heating element control)

## Setup

1. **Update Passwords** (optional but recommended)
   - Open `incubator.ino`
   - Find the "Configuration - Passwords" section at the top
   - **IMPORTANT**: `AP_PASS` must be at least 8 characters long for WPA2 security. If less than 8 characters, the device will broadcast as "ESP_XXXXXXX" instead of "Incubator"
   - Change `AP_PASS` to at least 8 characters (default: `12345678`)
   - Change `OTA_PASS` if desired (default: `1234`)

2. **Hardware Connections**
   - DHT22 sensor on GPIO pin 4
   - Relay on GPIO pin 5
   - Temperature alarm output on GPIO pin 6 (optional - goes HIGH when temp out of range)
   - Humidity alarm output on GPIO pin 7 (optional - goes HIGH when humidity out of range)
   - 3.3V and ground to DHT22 and Relay

3. **Install Libraries** (via Arduino Library Manager)
   - DHT sensor library (Adafruit)
   - Adafruit Unified Sensor
   - WebSockets (by Markus Sattler)

4. **Compile and Flash**
   - Select your ESP32 board in Arduino IDE
   - Compile and upload to device

## Running

### Initial Setup

1. **Connect to Access Point**
   - Device broadcasts its own wireless network: **"Incubator"**
   - **Note**: If you see "ESP_XXXXXXX" instead of "Incubator", the AP password is less than 8 characters. Update `AP_PASS` in the code to at least 8 characters and reflash.
   - Connect with password: **12345678** (or your custom 8+ character password)
   - Access web interface at: **http://10.0.0.1**

2. **Configure WiFi** (optional)
   - Click "WiFi Settings" link
   - Enter your WiFi network SSID and password
   - Choose whether to keep Access Point enabled when connected
   - Device will connect to your network and use DHCP for IP assignment
   - Access via the assigned IP address or continue using AP at 10.0.0.1

### Web Interface

The web interface displays:
- **Connection Status** - WiFi, WebSocket, IP addresses, MAC address
- **Sensor Readings** - Temperature (°F/°C), Humidity (%), Dew Point, Heat Index, Absolute Humidity
- **Lamp Status** - Visual indicator of heating element state
- **Temperature Range** - Configured target temperature range
- **Temperature Peaks** - Highest temperature peaks for multiple time windows (6h, 3h, 1.5h, 1h, 30m, 15m) with timestamps
- **Visual Status Indicators** - Color-coded temperature and humidity readings (green=in range, yellow=warning, red=alarm)

### Temperature Control

The device automatically controls the heating element (relay) based on:
- **Target Minimum** - Relay turns ON when temperature drops below this
- **Target Maximum** - Relay turns OFF when temperature reaches this
- **Safety Feature** - Relay turns OFF if sensor readings fail (safety measure)

**Alarm Thresholds:**
- **Temperature Alarm** - Triggers when temperature is outside target range by more than 0.5°F (GPIO pin 6 goes HIGH)
- **Humidity Alarm** - Triggers when humidity is outside target range by more than 5% (GPIO pin 7 goes HIGH)
- Default humidity target range: 40-60% (configurable via preferences)

### Egg Profiles

The incubator supports 37 bird species with optimized temperature ranges (listed alphabetically):

- **Chicken** - 98.0°F to 100.5°F (default)
- **Cockatiel** - 99.5°F to 100.0°F
- **Cormorant** - 99.0°F to 99.5°F
- **Crane** - 99.0°F to 99.5°F
- **Duck** - 99.5°F to 100.0°F
- **Duck Muscovy** - 99.0°F to 99.5°F
- **Eagle** - 99.0°F to 99.5°F
- **Emu** - 96.5°F to 97.5°F
- **Falcon** - 99.0°F to 99.5°F
- **Flamingo** - 99.0°F to 99.5°F
- **Goose** - 99.0°F to 99.5°F
- **Grouse** - 99.5°F to 100.0°F
- **Guinea Fowl** - 99.5°F to 100.0°F
- **Hawk** - 99.0°F to 99.5°F
- **Heron** - 99.0°F to 99.5°F
- **Hummingbird** - 99.5°F to 100.0°F
- **Large Parrots** (Macaw, Cockatoo) - 99.0°F to 99.5°F
- **Lovebird** - 99.5°F to 100.0°F
- **Ostrich** - 96.0°F to 97.0°F
- **Owl** - 99.0°F to 99.5°F
- **Parakeet** - 99.5°F to 100.0°F
- **Parrots** (Amazon, African Grey) - 99.5°F to 100.0°F
- **Partridge** - 99.5°F to 100.0°F
- **Peacock** - 99.5°F to 100.0°F
- **Pelican** - 99.0°F to 99.5°F
- **Penguin** - 98.5°F to 99.5°F
- **Pheasant** - 99.5°F to 100.0°F
- **Pigeon** - 99.5°F to 100.0°F
- **Quail** - 99.5°F to 100.5°F
- **Rail** - 99.0°F to 99.5°F
- **Rhea** - 97.0°F to 98.0°F
- **Seabirds** (Gull, Tern, Puffin) - 99.0°F to 99.5°F
- **Songbirds** (Finch, Canary, Sparrow) - 99.5°F to 100.0°F
- **Stork** - 99.0°F to 99.5°F
- **Swan** - 99.0°F to 99.5°F
- **Toucan** - 99.0°F to 99.5°F
- **Turkey** - 99.0°F to 100.0°F
- **Vulture** - 99.0°F to 99.5°F
- **Custom** - User-defined temperature range for any other species or specific requirements

Simply select your desired profile from the web interface dropdown, or use Custom mode to set your own temperature range. The device will automatically maintain the selected temperature range throughout the incubation period.

### OTA Updates

1. Connect device to your network
2. In Arduino IDE: Tools → Port → Select "Incubator at [IP address]"
3. Upload new firmware over-the-air
4. **Note**: During OTA updates, the Access Point is automatically disabled to ensure stable connection. The device will restart after a successful update.

## API Endpoints

- `GET /` - Main web interface
- `GET /wifi` - WiFi configuration page
- `GET /api/wifi` - Get current WiFi settings
- `POST /api/wifi` - Update WiFi settings
- `GET /api/wifi/scan` - Scan for available networks
- `POST /api/profile` - Update egg profile and temperature range
- `POST /api/reset` - Reboot device

## WebSocket

- **Port**: 81
- **Protocol**: `ws://[device-ip]:81/`
- **Updates**: Broadcasts sensor data every 2 seconds
- **Format**: JSON with temperature, humidity, lamp status, and configuration

## License

Copyright (c) 2025 John Minton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to use,
copy, modify, and distribute the Software for **non-commercial purposes only**,
subject to the following conditions:

1. The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

2. **Commercial use is prohibited** without express written permission from the
   copyright holder.

3. The Software is provided "AS IS", without warranty of any kind, express or
   implied, including but not limited to the warranties of merchantability,
   fitness for a particular purpose and noninfringement. In no event shall the
   authors or copyright holders be liable for any claim, damages or other
   liability, whether in an action of contract, tort or otherwise, arising from,
   out of or in connection with the Software or the use or other dealings in the
   Software.

**For commercial use inquiries, please contact:** cjohnweb@gmail.com

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues 
for bugs and feature requests.

