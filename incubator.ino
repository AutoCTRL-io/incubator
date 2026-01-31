#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "appstate_module.h"
#include "loader_module.h"
#include "wifi_module.h"
#include "webserver_module.h"
#include "dht_module.h"
#include "ota_module.h"
#include "profiles_module.h"
#include "core_module.h"
#include "ws_module.h"

WebServer server(80);
WebSocketsServer ws(81);

/* Pins per excluded/original_code.ino: DHT 4, relay 5. Stepper not in use. */
#define DHT_PIN 4
#define RELAY_PIN 5
/* Core-driven pipeline: temp read -> decision engine -> lamp state -> WebSocket every 2.05s. */
static const uint32_t SENSOR_BROADCAST_INTERVAL_MS = 2050;
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
  core_setRelayPin(RELAY_PIN);  /* Core drives relay (lamp) directly. */
  ws_setup(ws);

  Serial.println("(WiFi IP every 10s; DHT/decision/lamp/WS every 2.05s; OTA enabled)");
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
  ws_loop(ws);

  unsigned long now = millis();
  if (now - lastSensorBroadcastMs >= SENSOR_BROADCAST_INTERVAL_MS) {
    lastSensorBroadcastMs = now;
    SensorReadings sr;
    bool valid = getLastSensorReadings(sr);
    coreUpdate(sr);  /* Decision engine: process state, targets, lamp state; drives relay. */
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
