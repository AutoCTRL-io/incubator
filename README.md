# ESP32 Chicken Egg Incubator
### John Minton <cjohnweb@gmail.com>

An ESP32-S3 based incubator controller with web interface, real-time sensor monitoring, and automatic temperature control.

## Features

- **DHT22 Temperature & Humidity Sensor** - Real-time monitoring
- **Automatic Temperature Control** - Relay-based heating control
- **Web Interface** - Modern dark-themed UI accessible via browser
- **WebSocket Real-time Updates** - Live sensor data streaming
- **WiFi Configuration** - Easy setup via web interface
- **OTA Updates** - Over-the-air firmware updates
- **Multiple Egg Profiles** - Pre-configured settings for different bird species
- **Temperature Peak Tracking** - Records top temperature peaks over time windows
- **Dual WiFi Mode** - Access Point + Station mode for always-accessible setup

## Hardware Requirements

- ESP32 or S3 development board
- DHT22 temperature/humidity sensor
- Relay module (for heating element control)

## Setup

1. **Update Passwords** (optional)
   - Open `incubator.ino`
   - Find the "Configuration - Passwords" section at the top
   - Change `AP_PASS` and `OTA_PASS` if desired (both default to `1234`)

2. **Hardware Connections**
   - DHT22 sensor on GPIO pin 4
   - Relay on GPIO pin 5
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
   - Connect with password: **1234** (or your custom password)
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
- **Temperature Peaks** - Top 6 temperature peaks with timestamps

### Temperature Control

The device automatically controls the heating element (relay) based on:
- **Target Minimum** - Relay turns ON when temperature drops below this
- **Target Maximum** - Relay turns OFF when temperature reaches this
- **Safety Feature** - Relay turns OFF if sensor readings fail (safety measure)

### Egg Profiles

Pre-configured profiles available:
- **Chicken** - 98.0°F to 100.5°F (default)
- **Quail** - 99.5°F to 101.0°F
- **Duck** - 99.5°F to 100.0°F
- **Turkey** - 99.0°F to 100.0°F
- **Goose** - 99.0°F to 100.0°F
- **Custom** - User-defined temperature range

### OTA Updates

1. Connect device to your network
2. In Arduino IDE: Tools → Port → Select "Incubator at [IP address]"
3. Upload new firmware over-the-air

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

