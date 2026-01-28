#pragma once

#include <Arduino.h>

/*
  OTA Manager
  -----------
  Handles OTA update lifecycle.
  Exposes a single flag to block control logic during updates.
*/

void otaInit();
void otaLoop();

extern volatile bool otaInProgress;
