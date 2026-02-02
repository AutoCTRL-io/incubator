# Incubator – Project Status

## Current build: full pipeline (DHT → core → lamp/humidifier → WebSocket)

**Entry point:** `incubator.ino`  
**Active modules:** appstate, loader, wifi, webserver, dht, ota, profiles, core, turning, stepper (stub), ws  
**Pins:** DHT = GPIO 4, Lamp = GPIO 5, Humidifier = GPIO 6  

## Sensor and broadcast

- **DHT:** Read every **2 s** in background task (`dht_module`).  
- **Main loop:** Every **2 s** (`SENSOR_BROADCAST_INTERVAL_MS`): `getLastSensorReadings(sr)` → `coreUpdate(sr)` → `wsBroadcastStatus(sr)`. Lamp and humidifier state are derived from temp/humidity in `climate_module` and applied to GPIO in `core_module`.  
- **On egg turn:** After `turning_loop()`, if `turning_didTurnLastLoop()`: immediate `coreUpdate(sr)` + `wsBroadcastStatus(sr)` so frontend gets new tilt position and time-until-next.

## WebSocket

- **Status payload:** `type: "status"` with temp, humidity, lamp, **humidifier**, **tilt_position** (left/center/right), **rotation_enabled**, **motor_seconds_until_next**, motor_last_turn, process/profile/day, targets, etc.  
- **On client connect:** Send `info` then **status** (from last stored sensor/state) so the page has full state on load.  
- **On change:** Status is broadcast every 2 s and again when a turn occurs.

## Lamp and humidifier

- **core_module:** `core_setLampPin(5)`, `core_setHumidifierPin(6)`. No "relay" naming; only lamp and humidifier.  
- **climate_module:** Lamp on when temp &lt; targetMin, off when temp ≥ targetMax. Humidifier on when humidity &lt; hmin, off when humidity ≥ hmax.  
- **appstate:** `appstate_setLamp()` / `appstate_setHumidifier()`; both cleared when process inactive or system/control disabled.

## Egg turning

- **turning_module:** Configured by core from active phase (`turningEnabled`, `turnIntervalHours`). `turning_loop()` runs each main loop; when interval elapsed, calls `stepperTurnOnce()` and `markEggsTurned()`.  
- **turning_didTurnLastLoop():** Returns true once after a turn (then cleared); main loop uses this to broadcast status immediately.  
- **Status:** `rotation_enabled`, `tilt_position` (left/center/right from motor phase 0–360° in thirds), `motor_seconds_until_next`, `motor_last_turn`.

## Frontend

- **Right column:** Process, Actions (System On/Off, Profile, Mode), manual targets when Custom, Turn every (hrs), Egg tilting (Rotation On/Off, Tilt, Next tilt in, Last tilt), Temperature Peaks.  
- **Left column:** Lamp and **Humidifier** state; temp/humidity and other metrics.  
- **applyStatus():** Handles `humidifier`, `tilt_position`, `rotation_enabled`, and existing status fields. Data is received on load (status sent on WS connect) and on every 2 s + on turn broadcast.

## Recent changes (this session)

- Temp/humidity read and reported every **2 s**; DHT task interval 2000 ms; broadcast interval 2000 ms.  
- Pins: DHT 4, **Lamp 5**, **Humidifier 6**; `core_setLampPin` / `core_setHumidifierPin` (no relay naming).  
- Humidifier: appstate getter/setter, climate logic (hmin/hmax), core applies to pin 6 and clears when not managed.  
- Status includes `humidifier`, `tilt_position` (left/center/right), `rotation_enabled`; frontend shows Humidifier, Rotation, Tilt, Next tilt in, Last tilt.  
- WS: last status stored in `ws_module`; on connect send info + status; broadcast status on each 2 s tick and when `turning_didTurnLastLoop()`.

## Next steps (optional)

- Process UI: Start day editable when !active; Start/Cancel buttons and locking when active.  
- Custom profile UI: rotation on/off and "Turn every x hours" per phase.
