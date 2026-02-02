# Incubator – TODO

## Completed

- [x] Minimal build: Serial + WiFi AP + Web (pages + static)
- [x] WiFi in `wifi_module` (AP, optional STA from NVS); loader for NVS/creds
- [x] core_module, dht_module, turning_module, stepper_module (stub), ws_module enabled
- [x] Temp/humidity read every 2 s; process and report over WS every 2 s
- [x] Pins: DHT 4, Lamp 5, Humidifier 6 (no "relay" naming)
- [x] Lamp and humidifier state in appstate; climate logic for both; core applies to GPIO
- [x] Status payload: lamp, humidifier, tilt_position (left/center/right), rotation_enabled, motor_seconds_until_next
- [x] WS: send status on client connect (from stored last status); broadcast on 2 s tick and on egg turn

## In progress

- (none)

## Pending

- [ ] Process UI: Start day editable when !active; Start/Cancel buttons and locking when active
- [ ] Custom profile UI: rotation on/off and "Turn every x hours" per phase
- [ ] Web UI or API to save WiFi STA credentials
- [ ] Integrate OTA when ready
