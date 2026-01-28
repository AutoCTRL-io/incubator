#include "ws_server.h"
#include "app_state.h"
#include "core_controller.h"
#include "motor_stepper.h"
#include <ArduinoJson.h>

static WebSocketsServer *wsPtr = nullptr;

void wsServerInit(WebSocketsServer &ws)
{
  wsPtr = &ws;
  ws.begin();
}

void wsServerLoop(WebSocketsServer &ws)
{
  ws.loop();
}

void wsBroadcastStatus(const SensorReadings &sensor)
{
  if (!wsPtr) return;

  StaticJsonDocument<1024> doc;

  // Process state
  doc["active"] = process.active;
  doc["profile_id"] = process.profileId;
  doc["process_type"] = process.processType;
  doc["day"] = process.currentDay;

  // Sensor data
  doc["temp_f"] = sensor.tempF;
  doc["temp_c"] = sensor.tempC;
  doc["rh"] = sensor.humidity;
  doc["ah"] = sensor.absHumidity;
  doc["dew_f"] = sensor.dewPointF;
  doc["heat_f"] = sensor.heatIndexF;

  // Targets
  doc["tmin"] = getActiveTargetMinF();
  doc["tmax"] = getActiveTargetMaxF();
  doc["hmin"] = getActiveHumMin();
  doc["hmax"] = getActiveHumMax();

  // Motor status
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
