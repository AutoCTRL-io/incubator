#pragma once

#include <Arduino.h>
#include <WebSocketsServer.h>
#include "dht_module.h"

void ws_setup(WebSocketsServer &ws);
void ws_loop(WebSocketsServer &ws);
/** Broadcast status. When sensorValid is false, use last known good sensor values so UI and lamp/humidifier stay unchanged. */
void wsBroadcastStatus(const SensorReadings &sensor, bool sensorValid);
