# Incubator – Project Status

## Current state (post-refactor)

**Entry:** `incubator.ino`  
**Modules:** appstate, loader, wifi, webserver, dht, ota, profiles, core, turning, stepper (stub), ws  
**Pins:** DHT = GPIO 4, Lamp = GPIO 5, Humidifier = GPIO 6  

### Implemented and working

- **WiFi:** AP + STA (NVS creds). STA MAC kept consistent (read after mode, re-apply after softAP). DHCP only; no static IP.
- **Sensor:** DHT read every 2 s in background task. On DHT fail: backend does not change lamp/humidifier; status uses last-good sensor; frontend keeps last-good values on screen.
- **Climate (mode-agnostic):** Receives ranges (tmin, tmax, hmin, hmax) + sensor. Lamp on when temp < tmin, off when temp ≥ tmax. Humidifier on when rh < hmin, off when rh ≥ hmax. No mode logic inside climate.
- **Core:** Resolves current targets from process (active phase) or manual (appstate). Single path: `resolveCurrentTargets` → `climate_setTargets` + `climate_update` → apply lamp/humidifier to GPIO. Manual targets stored in appstate and persisted (tmin/tmax/hmin/hmax in NVS); loader primes them at boot.
- **Lamp & humidifier:** Applied to GPIO 5 and 6 (on = LOW). Work in Manual, Egg Holding, and Incubation.
- **Egg turning:** turning_module drives stepper (stub). When tilting disabled (lockdown, manual off, process end), `stepperGoToFlat()` runs so platform returns to flat; status reports `tilt_position`: left / flat / right.
- **Web UI:** Mode first, then Profile/Preload. Empty profile option: clears presets, disables Start, egg tilting off. Start Incubation link when Egg Holding active; backend allows switch to Incubation. Communication timer indicator next to Overview header (small, no numbers). Disabled Start button: grey, no green.
- **WebSocket:** Status every 2 s and on turn; on connect send info + status (last-good sensor when current read invalid). Commands: set_mode, start_process, set_profile, set_manual_targets, set_turning, tilt_now, reset.
- **OTA:** ArduinoOTA in ota_module (setup + handle in loop). OTA password configurable on Settings page (stored in NVS; applied at boot).

---

## Outstanding for final release

### 1. **Stepper / motor (hardware)**

- **Status:** Logic and API in place; implementation is a **stub** (no real motor).
- **Remaining:** Wire real stepper (or tilt mechanism) to GPIO; implement in `stepper_module.cpp`: `stepper_loop()` (if needed), `stepperTurnOnce()` (physical steps), `stepperGoToFlat()` (drive to level position). Keep same API so core/turning unchanged.

### 2. **Web assets on device** (deferred to end)

- **Status:** UI lives in `web_assets/` (index.html, wifi.html, style.css, app.js). Webserver serves from **LittleFS** if files present; otherwise fallback “Upload at /upload”.
- **Remaining:** Before release, either:
  - Upload `web_assets/*` to device via **http://&lt;device&gt;/upload**, or  
  - Add a build step that flashes the same files into LittleFS (e.g. LittleFS upload in Arduino IDE or custom script).

### 3. **Documentation and release hygiene**

- **README** (in `excluded/`) is from an older design (relay naming, different pins, alarm GPIOs 6/7). Update or replace with current behavior: DHT 4, Lamp 5, Humidifier 6, no alarm pins, Mode/Profile/Preload, Start Incubation, flat position, etc.
- **PROJECT_STATUS.md** (this file) is now at repo root; consider archiving or trimming `excluded/PROJECT_STATUS.md` and `excluded/TODO.md` to avoid confusion.
- Optional: version string or build ID for OTA and support (e.g. in Serial banner or `/ping`).

### 4. **Optional / later**
- **Custom profile UI:** Per-phase rotation on/off and “Turn every x hours” (currently only preset profiles and manual turning).
- **WiFi credentials:** Already configurable via `/wifi` and NVS; confirm flow (connect to AP → set SSID/pass → device joins STA) and document in README.

---

## Summary

- **Firmware behavior:** Suitable for release once web assets are on the device and (when you’re ready) the stepper stub is replaced with real motor logic.
- **Must-do for release:** (1) Stepper hardware implementation when you’re ready, (2) get web assets onto device (upload or flash LittleFS), (3) update README to match current design.
- **Should-do:** Single, up-to-date PROJECT_STATUS (this file) and clear “remaining” list; optional versioning and OTA password.
