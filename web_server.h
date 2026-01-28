#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "app_state.h"

/*
  HTTP Web Server
  ---------------
  Serves HTML + REST endpoints.
  No WebSocket logic here.
*/

void webServerInit(WebServer &server);
void webServerLoop(WebServer &server);
