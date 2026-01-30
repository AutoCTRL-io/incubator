#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include "dht_module.h"
#include "appstate_module.h"

void ws_setup(WebSocketsServer &ws);
void ws_loop(WebSocketsServer &ws);
void wsBroadcastStatus(const SensorReadings &sensor);
