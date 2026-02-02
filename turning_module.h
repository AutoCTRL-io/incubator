#pragma once

#include <Arduino.h>

/*
  Egg turning module
  ------------------
  Runs on configuration and a timer only. Does not use DHT or appstate.
  Core tells it: enabled + interval hours (or disabled).
  It schedules turns and triggers the stepper when due.
*/

/** Configure turning: enabled and interval in hours (0 = disabled). Core calls this with active phase. */
void turning_configure(bool enabled, uint16_t intervalHours);

/** Call from main loop; checks timer and triggers turn when interval elapsed. */
void turning_loop();

/** Trigger one tilt immediately (e.g. from UI "Tilt now" button). Updates last-turn time. */
void turning_tiltNow();

/** True if a turn was performed on the last turning_loop() call; cleared when read. */
bool turning_didTurnLastLoop();
