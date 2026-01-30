#pragma once

#include <Arduino.h>
#include <WebServer.h>

/*
  HTTP Web Server
  ---------------
  Serves HTML + REST endpoints.
  No WebSocket logic here.
*/

void webserver_setup(WebServer &server);
void webserver_loop(WebServer &server);
