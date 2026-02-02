#pragma once

#include <Arduino.h>

/*
  Climate control module
  ----------------------
  Decides lamp and (later) humidifier state from temp/humidity vs targets.
  Does not read appstate or process; core passes targets and sensor data.
  Runs on its own: given targets and sensor, outputs lamp on/off (and later humidifier).
*/

struct SensorReadings;

/** Set target temp (°F) and humidity (%) ranges. Core calls this with active phase values. */
void climate_setTargets(float tempMinF, float tempMaxF, float humMin, float humMax);

/** Update internal state from sensor; computes lamp (and later humidifier) from targets. */
void climate_update(const SensorReadings &sensor);

/** Returns whether lamp should be on (from last climate_update). */
bool climate_getLampOn();

/** Returns whether humidifier should be on (placeholder; always false until implemented). */
bool climate_getHumidifierOn();
