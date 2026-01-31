#pragma once

#include <Arduino.h>

/*
  Loader Module
  -------------
  Runs after appstate_setup(). Reads NVS (namespace "incubator") from a previous
  run and, if data exists, overwrites appstate variables and stores WiFi credentials
  for wifi_setup to use. Single source of loaded persisted data.
*/

void loader_setup();
void loader_loop();

/* Call after loader_setup(); pass to wifi_setup for STA. getStaSsid() NULL = no saved credentials. */
const char* loader_getStaSsid();
/* When getStaSsid() is non-NULL, use this for password (may be "" for open network). */
const char* loader_getStaPass();
