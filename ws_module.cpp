#include "ws_module.h"
#include "appstate_module.h"
#include "core_module.h"
#include "stepper_module.h"
#include "turning_module.h"
#include <ArduinoJson.h>
#include <string.h>
#include <ESP.h>

static WebSocketsServer *wsPtr = nullptr;
static int wsClientCount = 0;

/* Last info JSON we sent; send again only when it changes. */
static char lastInfoJson[384];
static bool lastInfoJsonValid = false;

/* Last status payload; used to send status to newly connected clients. */
static SensorReadings lastSensorReadings;
static bool lastSensorReadingsValid = false;

/* Build info-section JSON from appstate (field names match frontend applyStatus). */
static void buildInfoDoc(JsonDocument &doc)
{
  doc["type"] = "info";
  doc["wifi_connected"] = appstate_getWifiConnected();
  doc["wifi_ssid"] = appstate_getStaSsid();
  doc["wifi_rssi"] = appstate_getWifiRssi();
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

/* Handle incoming WebSocket text (JSON command). All settings go over WS, not HTTP. */
static void wsHandleText(uint8_t *payload, size_t length)
{
  if (length == 0 || length > 512) return;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;

  const char *cmd = doc["type"] | doc["cmd"] | "";

  if (strcmp(cmd, "set_system") == 0) {
    bool enabled = doc["enabled"] | true;
    appstate_setSystemEnabled(enabled);
    saveProcessState();
  } else if (strcmp(cmd, "set_mode") == 0) {
    int v = doc["mode"] | 0;
    if (v == 0) {
      cancelProcess();
    } else if (v == 1) {
      uint16_t startDay = doc["start_day"] | process.startDay;
      startProcess(PROCESS_EGG_HOLDING, process.profileId, startDay);
    } else if (v == 2) {
      uint16_t startDay = doc["start_day"] | process.startDay;
      startProcess(PROCESS_INCUBATION, process.profileId, startDay);
    }
  } else if (strcmp(cmd, "start_process") == 0) {
    if (process.active) return;
    int mode = doc["mode"] | 1;
    uint16_t startDay = doc["start_day"] | process.startDay;
    if (mode == 1)
      startProcess(PROCESS_EGG_HOLDING, process.profileId, startDay);
    else if (mode == 2)
      startProcess(PROCESS_INCUBATION, process.profileId, startDay);
  } else if (strcmp(cmd, "set_start_day") == 0) {
    if (!process.active) {
      process.startDay = doc["start_day"] | 0;
      saveProcessState();
    }
  } else if (strcmp(cmd, "set_profile") == 0) {
    int id = doc["profile_id"] | 0;
    if (id >= 0 && id <= (int)PROFILE_CUSTOM) {
      process.profileId = (uint8_t)id;
      currentProfile = (EggProfileId)id;
      saveProcessState();
    }
  } else if (strcmp(cmd, "set_manual_targets") == 0) {
    float tmin = doc["tmin"] | 0.0f;
    float tmax = doc["tmax"] | 0.0f;
    float hmin = doc["hmin"] | 0.0f;
    float hmax = doc["hmax"] | 0.0f;
    appstate_setManualTargets(tmin, tmax, hmin, hmax);
  } else if (strcmp(cmd, "set_turning") == 0) {
    /* Manual mode: save user's egg tilting on/off and interval; persist so it survives reboot. */
    bool enabled = doc["enabled"] | false;
    int turnsPerDay = doc["turns_per_day"] | 0;
    uint16_t intervalHours = 2;
    if (enabled && turnsPerDay > 0 && turnsPerDay <= 24)
      intervalHours = (uint16_t)(24 / turnsPerDay);
    if (intervalHours < 1) intervalHours = 1;
    if (intervalHours > 24) intervalHours = 24;
    appstate_setManualTurning(enabled, enabled ? intervalHours : (uint16_t)2);
    saveProcessState();
    /* Push status immediately so UI gets confirmation (rotation_enabled / turn_interval_hours). */
    if (lastSensorReadingsValid)
      wsBroadcastStatus(lastSensorReadings);
  } else if (strcmp(cmd, "tilt_now") == 0) {
    /* Manual tilt: trigger one turn and update last-turn time; broadcast so UI updates. */
    turning_tiltNow();
    if (lastSensorReadingsValid)
      wsBroadcastStatus(lastSensorReadings);
  } else if (strcmp(cmd, "reset") == 0) {
    ESP.restart();
  }
}

/* Map motor phase (0–360 degrees) to tilt position: left, center, right (thirds). */
static const char *tiltPositionFromPhase(float phase)
{
  if (phase < 120.0f) return "left";
  if (phase < 240.0f) return "center";
  return "right";
}

/* Build status JSON from sensor and current process/motor state (for broadcast or single-client send). */
static void buildStatusDoc(JsonDocument &doc, const SensorReadings &sensor)
{
  doc["type"] = "status";

  doc["active"] = process.active;
  doc["profile_id"] = process.profileId;
  doc["process_type"] = process.processType;
  doc["day"] = process.currentDay;
  doc["start_day"] = process.startDay;
  doc["active_phase_index"] = process.activePhaseIndex;
  if (process.active) {
    const IncubationPhase *phase = getActivePhase();
    doc["rotation_enabled"] = phase->turningEnabled;
    doc["turn_interval_hours"] = phase->turnIntervalHours;
  } else {
    doc["rotation_enabled"] = appstate_getManualTurningEnabled();
    doc["turn_interval_hours"] = appstate_getManualTurnIntervalHours();
  }

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
  doc["tilt_position"] = tiltPositionFromPhase(motor.rotationPhase);
  doc["motor_last_turn"] = (uint64_t)motor.lastTurnEpoch;
  doc["motor_turns_per_day"] = motor.turnsPerDay;
  doc["motor_seconds_until_next"] = motor.secondsUntilNextTurn;
  doc["rotation_per_turn"] = 360;

  doc["lamp"] = appstate_getLamp();
  doc["humidifier"] = appstate_getHumidifier();
  doc["system_enabled"] = appstate_getSystemEnabled();
}

/* Send last known status to one client (e.g. on connect). No-op if no status yet. */
static void wsSendStatusToClient(uint8_t num)
{
  if (!wsPtr || !lastSensorReadingsValid) return;
  StaticJsonDocument<1024> doc;
  buildStatusDoc(doc, lastSensorReadings);
  char buf[1024];
  size_t len = serializeJson(doc, buf);
  wsPtr->sendTXT(num, buf, len);
}

static void wsOnEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_CONNECTED) {
    wsClientCount++;
    appstate_setWsConnected(true);
    wsSendInfoToClient(num);
    wsSendStatusToClient(num);
  } else if (type == WStype_DISCONNECTED) {
    if (wsClientCount > 0) wsClientCount--;
    appstate_setWsConnected(wsClientCount > 0);
  } else if (type == WStype_TEXT) {
    wsHandleText(payload, length);
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

  lastSensorReadings = sensor;
  lastSensorReadingsValid = true;

  wsBroadcastInfoIfChanged();

  StaticJsonDocument<1024> doc;
  buildStatusDoc(doc, sensor);

  char buf[1024];
  size_t len = serializeJson(doc, buf);
  wsPtr->broadcastTXT(buf, len);
}
