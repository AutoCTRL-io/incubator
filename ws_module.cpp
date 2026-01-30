#include <Arduino.h>
#include <WiFi.h>
#include "ws_module.h"
#include "appstate_module.h"
#include "core_module.h"
#include "stepper_module.h"
#include <ArduinoJson.h>

static WebSocketsServer *wsPtr = nullptr;

void ws_setup(WebSocketsServer &ws)
{
  wsPtr = &ws;
  ws.begin();
}

void ws_loop(WebSocketsServer &ws)
{
  ws.loop();
}

void wsBroadcastStatus(const SensorReadings &sensor)
{
  if (!wsPtr) return;

  StaticJsonDocument<1024> doc;

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

  char buf[1024];
  size_t len = serializeJson(doc, buf);
  wsPtr->broadcastTXT(buf, len);
}
