# Incubator – Project Status

## Current build: minimal (Serial + WiFi AP + Web)

**Entry point:** `incubator.ino`  
**Active modules:** `appstate_module`, `loader_module`, `wifi_module`, `webserver_module`  
**Startup order:** appstate_setup() (defaults only) → loader_setup() (NVS load, overwrite appstate, store wifi creds) → wifi_setup(..., loader creds) → webserver_setup()  
**Served:** `/`, `/wifi`, static assets (`/style.css`, `/app.js`)  
**Not yet in use:** `ota_module`, `core_module`, `dht_module`, `stepper_module`, `ws_module`, `profiles_module`, `webassets_module` (full UI)

## WiFi behavior

- **AP:** Always started; SSID "Incubator", default pass "12345678". AP IP: `192.168.4.1`.
- **STA:** Only used if NVS has saved credentials (`wifi_module` loads them). If none, mode is AP-only and **WiFi IP (STA) = 0.0.0.0** — this is expected. **WiFi IP is printed every 1 second in `wifi_loop()`** so you can see when/if STA gets an address.
- "WiFi: SUCCESS" means the **Access Point** started; it does not mean the device connected to your home WiFi as a client.

## OTA

- **OTA is not included in the minimal build.** The `ota_module` exists but is not called from `incubator.ino` yet.

## Architecture (full app, for when modules are re-enabled)

- **appstate_module:** Global `process` struct and targets; `appstate_setup()` sets defaults only (no NVS read). `saveProcessState()` still writes. Load is done by loader_module.
- **loader_module:** Single place that reads NVS (namespace "incubator"). Runs after appstate_setup(); overwrites appstate vars if data exists; retrieves WiFi credentials for wifi_setup. `loader_setup`/`loader_loop`; `loader_getStaSsid`/`loader_getStaPass`.
- **core_module:** Start/cancel/transition process, day tracking, target resolution, egg-turning logic; `core_setup`/`core_loop`.
- **stepper_module:** Motor position/phase, `stepperSetTurnsPerDay`; `stepper_setup`/`stepper_loop`.
- **webserver_module (full):** All API endpoints when restored; currently pages + static only; `webserver_setup`/`webserver_loop`.
- **ws_module:** WebSockets; motor status in broadcast; `ws_setup`/`ws_loop`.
- **profiles_module:** 38 egg profiles; `profiles_setup`/`profiles_loop`.
- **dht_module:** DHT22 temp/humidity; `dht_setup`/`dht_loop`.
- **wifi_module:** AP/STA; `wifi_setup(apSsid, apPass, staSsid, staPass)` — STA credentials come from loader (no NVS read in wifi). `wifi_loop()` prints AP/STA IP every 1s. `wifiSaveCredentials()` still writes to NVS for next boot.
- **ota_module:** OTA updates; `ota_setup`/`ota_loop`.
- **webassets_module:** HTML/CSS/JS blobs; `webassets_setup`/`webassets_loop`.

## Recent changes

- **Loader added:** appstate runs first (defaults only); loader runs second and loads NVS, overwrites appstate, retrieves WiFi credentials; wifi uses loader creds (no NVS read in wifi for creds).
- All modules use `x_setup()`/`x_loop()`; incubator.ino calls appstate_setup → loader_setup → wifi_setup(loader creds) → webserver_setup.
- `wifi_loop()` prints AP IP and STA IP every 1 second to serial.
- Web server reduced to pages + static for minimal test; API handlers removed from this build.
- Serial log shows AP IP and STA IP; STA = 0.0.0.0 when no saved credentials.
- OTA not included in minimal build (reverted).

## Next steps (when expanding from minimal)

1. Add UI/API to save WiFi STA credentials (then STA IP will show after reconnect).
2. Re-enable modules incrementally: appstate_module → core_module → dht_module → stepper_module → ws_module, etc.
3. Restore full webserver_module API and webassets_module when core is stable.
