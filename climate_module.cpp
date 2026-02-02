/*
  Climate control: temp/humidity vs targets → lamp (and later humidifier) state.
  Core tells us the targets; we only see sensor data and targets. No appstate.
*/

#include "climate_module.h"
#include "dht_module.h"
#include <math.h>

static float s_tempMinF = NAN;
static float s_tempMaxF = NAN;
static float s_humMin = NAN;
static float s_humMax = NAN;
static bool s_lampOn = false;
static bool s_humidifierOn = false;

void climate_setTargets(float tempMinF, float tempMaxF, float humMin, float humMax)
{
  s_tempMinF = tempMinF;
  s_tempMaxF = tempMaxF;
  s_humMin = humMin;
  s_humMax = humMax;
}

void climate_update(const SensorReadings &sensor)
{
  float tempF = sensor.tempF;
  if (isnan(tempF) || isnan(s_tempMinF) || isnan(s_tempMaxF)) {
    s_lampOn = false;
    s_humidifierOn = false;
    return;
  }
  if (tempF < s_tempMinF)
    s_lampOn = true;
  else if (tempF >= s_tempMaxF)
    s_lampOn = false;
  /* else leave lamp state unchanged (hysteresis in-band) */

  /* Humidifier: on when below min, off when at or above max (hysteresis in-band). */
  float rh = sensor.humidity;
  if (!isnan(rh) && !isnan(s_humMin) && !isnan(s_humMax)) {
    if (rh < s_humMin)
      s_humidifierOn = true;
    else if (rh >= s_humMax)
      s_humidifierOn = false;
  } else {
    s_humidifierOn = false;
  }
}

bool climate_getLampOn()
{
  return s_lampOn;
}

bool climate_getHumidifierOn()
{
  return s_humidifierOn;
}
