#pragma once

#include <Arduino.h>
#include <WebSocketsServer.h>
#include "sensor_dht.h"
#include "app_state.h"

void wsServerInit(WebSocketsServer &ws);
void wsServerLoop(WebSocketsServer &ws);
void wsBroadcastStatus(const SensorReadings &sensor);
