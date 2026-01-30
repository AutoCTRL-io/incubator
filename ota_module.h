#pragma once

#include <Arduino.h>

/*
  OTA Manager
  -----------
  Handles OTA update lifecycle.
  Exposes a single flag to block control logic during updates.
*/

void ota_setup();
void ota_loop();

extern volatile bool otaInProgress;
