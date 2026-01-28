// web_server.cpp
#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include "web_server.h"
#include "web_assets.h"
#include "app_state.h"
#include "core_controller.h"

static WebServer *gServer = nullptr;

static void sendJson(int code, const JsonDocument &doc)
{
  String out;
  serializeJson(doc, out);
  gServer->send(code, "application/json", out);
}

static void handleGetState()
{
  StaticJsonDocument<512> doc;

  doc["active"] = process.active;
  doc["controlMode"] = (uint8_t)process.controlMode;
  doc["processType"] = (uint8_t)process.processType;
  doc["profileId"] = process.profileId;
  doc["startEpoch"] = (uint64_t)process.startEpoch;
  doc["startDay"] = process.startDay;
  doc["currentDay"] = process.currentDay;
  doc["lastTurnEpoch"] = (uint64_t)process.lastTurnEpoch;
  doc["customMinF"] = process.customMinF;
  doc["customMaxF"] = process.customMaxF;
  doc["customHumMin"] = process.customHumMin;
  doc["customHumMax"] = process.customHumMax;
  doc["customTotalDays"] = process.customTotalDays;
  doc["customTurnsPerDay"] = process.customTurnsPerDay;

  sendJson(200, doc);
}

static bool parseBody(JsonDocument &doc)
{
  const String body = gServer->arg("plain");
  if (body.length() == 0) return false;

  DeserializationError err = deserializeJson(doc, body);
  return !err;
}

static void handleStart()
{
  StaticJsonDocument<512> body;
  if (!parseBody(body))
  {
    gServer->send(400, "application/json", "{\"ok\":false,\"err\":\"bad_json\"}");
    return;
  }

  // expected:
  //  { "type": <int>, "profileId": <int>, "startDay": <int> }
  const uint8_t type = body["type"] | (uint8_t)PROCESS_EGG_HOLDING;
  const uint8_t profileId = body["profileId"] | 0;
  const uint16_t startDay = body["startDay"] | 1;

  const bool ok = startProcess((ProcessType)type, profileId, startDay);

  StaticJsonDocument<128> resp;
  resp["ok"] = ok;
  if (!ok) resp["err"] = "start_failed";
  sendJson(ok ? 200 : 400, resp);
}

static void handleCancel()
{
  cancelProcess();
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleTransition()
{
  const bool ok = transitionProcess();
  StaticJsonDocument<128> resp;
  resp["ok"] = ok;
  if (!ok) resp["err"] = "transition_failed";
  sendJson(ok ? 200 : 400, resp);
}

static void handleReset()
{
  resetProcessState();
  gServer->send(200, "application/json", "{\"ok\":true}");
}

void webServerInit(WebServer &server)
{
  gServer = &server;

  /* ===== Pages ===== */
  server.on("/", HTTP_GET, [&]() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/wifi", HTTP_GET, [&]() {
    server.send_P(200, "text/html", WIFI_HTML);
  });

  /* ===== Static Assets ===== */
  server.on("/style.css", HTTP_GET, [&]() {
    server.send_P(200, "text/css", STYLES);
  });

  server.on("/app.js", HTTP_GET, [&]() {
    server.send_P(200, "application/javascript", JAVASCRIPTS);
  });

  /* ===== API ===== */
  server.on("/api/state", HTTP_GET, handleGetState);
  server.on("/api/process/start", HTTP_POST, handleStart);
  server.on("/api/process/cancel", HTTP_POST, handleCancel);
  server.on("/api/process/transition", HTTP_POST, handleTransition);
  server.on("/api/reset", HTTP_POST, handleReset);

  server.begin();
}

void webServerLoop(WebServer &server)
{
  server.handleClient();
}
