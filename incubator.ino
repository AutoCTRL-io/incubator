#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <time.h>
#include "appstate_module.h"
#include "loader_module.h"
#include "wifi_module.h"
#include "webserver_module.h"
#include "dht_module.h"
#include "ota_module.h"
#include "profiles_module.h"
#include "core_module.h"
#include "turning_module.h"
#include "stepper_module.h"
#include "ws_module.h"

WebServer server(80);
WebSocketsServer ws(81);

/* Pins: DHT 4, lamp 5, humidifier 6. Stepper not in use. */
#define DHT_PIN 4
#define LAMP_PIN 5
#define HUMIDIFIER_PIN 6
/* Core-driven pipeline: temp/humidity read -> process -> lamp/humidifier -> WebSocket every 2s. */
static const uint32_t SENSOR_BROADCAST_INTERVAL_MS = 2000;
static unsigned long lastSensorBroadcastMs = 0;

/* ===== appstate -> loader -> wifi -> webserver + dht + ota + profiles + core + ws (stepper is stub) ===== */
void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n=== Incubator - WiFi + Web ===");
  Serial.flush();

  appstate_setup();   /* Defaults only; no NVS read */
  loader_setup();     /* Load from NVS; overwrite appstate; store wifi creds for wifi_setup */

  const char *apSsid = "Incubator";
  const char *staSsid = loader_getStaSsid();
  if (wifi_setup(apSsid, "12345678", staSsid, loader_getStaPass())) {
    appstate_setApSsid(apSsid);
    if (staSsid && staSsid[0]) appstate_setStaSsid(staSsid);
    Serial.println("WiFi: SUCCESS");
    Serial.print("AP IP: ");
    Serial.println(wifiGetAPIP());
    Serial.print("WiFi IP: ");
    Serial.println(wifiGetSTAIP());
    /* Sync time from NTP when STA is connected (device uses UTC; UI shows local time). */
    if (WiFi.status() == WL_CONNECTED) {
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      struct tm t;
      for (int i = 0; i < 15 && !getLocalTime(&t); i++) delay(500);
      Serial.println(getLocalTime(&t) ? "Time: NTP synced (UTC)" : "Time: NTP pending (use UTC when synced)");
    } else {
      Serial.println("Time: no STA, using RTC/boot (UTC when NTP was last synced)");
    }
  } else {
    Serial.println("WiFi: FAILED");
  }
  Serial.flush();

  webserver_setup(server);
  Serial.println("Web server: started");
  Serial.println("  On STA network (same WiFi as device): use http://<STA_IP> e.g. http://192.168.x.x");
  Serial.print("  On Incubator AP: use http://");
  Serial.println(wifiGetAPIP());
  Serial.print("  STA IP for this device: ");
  Serial.println(wifiGetSTAIP());
  Serial.println("  Test: http://<IP>/ping");
  Serial.flush();

  dht_setup(DHT_PIN);
  ota_setup();
  profiles_setup();
  core_setup();
  core_setLampPin(LAMP_PIN);
  core_setHumidifierPin(HUMIDIFIER_PIN);
  { StepperConfig sc = { 0, 0, 0, 360, false }; stepper_setup(sc); }  /* Stub until real motor. */
  /* Restore last-turn time from NVS when clock is synced so tilting state survives reboot (with NTP). */
  {
    time_t now = time(nullptr);
    if (now >= 100000 && process.lastTurnEpoch > 0)
      stepper_setLastTurnEpoch(process.lastTurnEpoch);
  }
  ws_setup(ws);

  Serial.println("(WiFi IP every 10s; sensor/core/WS every 2s; OTA enabled)");
  Serial.flush();
}

void loop()
{
  wifi_loop();
  webserver_loop(server);
  ota_loop();
  profiles_loop();
  dht_loop();
  core_loop();
  turning_loop();   /* Timer-based; no DHT. Core tells it enabled/interval. */
  /* On turn: push status immediately so frontend gets new tilt and time-until-next. */
  if (turning_didTurnLastLoop()) {
    SensorReadings sr;
    if (getLastSensorReadings(sr)) {
      coreUpdate(sr);
      wsBroadcastStatus(sr);
    }
  }
  ws_loop(ws);

  unsigned long now = millis();
  if (now - lastSensorBroadcastMs >= SENSOR_BROADCAST_INTERVAL_MS) {
    lastSensorBroadcastMs = now;
    SensorReadings sr;
    bool valid = getLastSensorReadings(sr);
    coreUpdate(sr);  /* Orchestrator: phase → climate targets + turning config; climate decides lamp; relay applied here. */
    wsBroadcastStatus(sr);
    if (valid) {
      Serial.print("DHT: ");
      Serial.print(sr.tempF);
      Serial.print(" F, ");
      Serial.print(sr.humidity);
      Serial.println("% RH");
    } else {
      Serial.println("DHT: no reading");
    }
    Serial.flush();
  }
}
