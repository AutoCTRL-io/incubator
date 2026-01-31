#include "ws_module.h"
#include "appstate_module.h"
#include "core_module.h"
#include "stepper_module.h"
#include <ArduinoJson.h>
#include <string.h>

static WebSocketsServer *wsPtr = nullptr;
static int wsClientCount = 0;

/* Last info JSON we sent; send again only when it changes. */
static char lastInfoJson[384];
static bool lastInfoJsonValid = false;

/* Build info-section JSON from appstate (field names match frontend applyStatus). */
static void buildInfoDoc(JsonDocument &doc)
{
  doc["type"] = "info";
  doc["wifi_connected"] = appstate_getWifiConnected();
  doc["wifi_ssid"] = appstate_getStaSsid();
  doc["ap_ssid"] = appstate_getApSsid();
  doc["ws"] = appstate_getWsConnected() ? "CONNECTED" : "DISCONNECTED";
  doc["mode"] = appstate_getDisplayMode();
  doc["ip_ap"] = appstate_getApIp();
  doc["ip_sta"] = appstate_getWifiIp();
  doc["mac"] = appstate_getMac();
}

/* Send current info to one client (e.g. on connect). */
static void wsSendInfoToClient(uint8_t num)
{
  if (!wsPtr) return;
  StaticJsonDocument<384> doc;
  buildInfoDoc(doc);
  char buf[384];
  size_t len = serializeJson(doc, buf);
  wsPtr->sendTXT(num, buf, len);
}

/* Broadcast info to all clients if it changed. */
static void wsBroadcastInfoIfChanged()
{
  if (!wsPtr || wsClientCount == 0) return;
  StaticJsonDocument<384> doc;
  buildInfoDoc(doc);
  char buf[384];
  size_t len = serializeJson(doc, buf);
  buf[len] = '\0';
  if (!lastInfoJsonValid || strcmp(buf, lastInfoJson) != 0) {
    wsPtr->broadcastTXT(buf, len);
    strncpy(lastInfoJson, buf, sizeof(lastInfoJson) - 1);
    lastInfoJson[sizeof(lastInfoJson) - 1] = '\0';
    lastInfoJsonValid = true;
  }
}

static void wsOnEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  (void)payload;
  (void)length;
  if (type == WStype_CONNECTED) {
    wsClientCount++;
    appstate_setWsConnected(true);
    wsSendInfoToClient(num);
  } else if (type == WStype_DISCONNECTED) {
    if (wsClientCount > 0) wsClientCount--;
    appstate_setWsConnected(wsClientCount > 0);
  }
}

void ws_setup(WebSocketsServer &ws)
{
  wsPtr = &ws;
  ws.onEvent(wsOnEvent);
  ws.begin();
}

void ws_loop(WebSocketsServer &ws)
{
  ws.loop();
}

void wsBroadcastStatus(const SensorReadings &sensor)
{
  if (!wsPtr) return;

  wsBroadcastInfoIfChanged();

  StaticJsonDocument<1024> doc;
  doc["type"] = "status";

  doc["active"] = process.active;
  doc["profile_id"] = process.profileId;
  doc["process_type"] = process.processType;
  doc["day"] = process.currentDay;

  doc["temp_f"] = sensor.tempF;
  doc["temp_c"] = sensor.tempC;
  doc["rh"] = sensor.humidity;
  doc["ah"] = sensor.absHumidity;
  doc["dew_f"] = sensor.dewPointF;
  doc["heat_f"] = sensor.heatIndexF;

  doc["tmin"] = getActiveTargetMinF();
  doc["tmax"] = getActiveTargetMaxF();
  doc["hmin"] = getActiveHumMin();
  doc["hmax"] = getActiveHumMax();

  MotorStatus motor = stepperGetStatus();
  doc["motor_position"] = motor.absolutePosition;
  doc["motor_phase"] = motor.rotationPhase;
  doc["motor_last_turn"] = (uint64_t)motor.lastTurnEpoch;
  doc["motor_turns_per_day"] = motor.turnsPerDay;
  doc["motor_seconds_until_next"] = motor.secondsUntilNextTurn;

  doc["lamp"] = appstate_getLamp();

  char buf[1024];
  size_t len = serializeJson(doc, buf);
  wsPtr->broadcastTXT(buf, len);
}
