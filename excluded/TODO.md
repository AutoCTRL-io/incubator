# Incubator – TODO

## Completed

- [x] Minimal build: Serial + WiFi AP + Web (pages + static)
- [x] WiFi in `wifi_module` (AP, optional STA from NVS); `wifi_loop()` prints WiFi IP every 1s
- [x] Serial log: AP IP, STA IP, web server URLs
- [x] Loader module: appstate runs first (defaults), loader runs second (NVS load, overwrite appstate, retrieve wifi creds)
- [x] appstate_setup in main (defaults only); loader_setup in main; wifi uses loader creds

## In progress

- (none)

## Pending

- [ ] Integrate OTA (`ota_module`) when ready
- [ ] Web UI or API to save WiFi STA credentials (SSID/password to NVS; loader will pick up on next boot)
- [ ] Re-enable `core_module` (core_setup/loop)
- [ ] Re-enable `dht_module` (dht_setup/loop)
- [ ] Re-enable `stepper_module` (stepper_setup/loop)
- [ ] Re-enable `ws_module` (ws_setup/loop)
- [ ] Restore full `webserver_module` API and `webassets_module` UI
- [ ] Lamp/relay control (if required)
