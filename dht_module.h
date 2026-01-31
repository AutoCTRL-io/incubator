#pragma once
#include <Arduino.h>

/* =========================
   Sensor Data Model
   ========================= */

struct SensorReadings {
  float tempC;
  float tempF;
  float humidity;
  float absHumidity;
  float dewPointC;
  float dewPointF;
  float heatIndexC;
  float heatIndexF;
};

/* =========================
   Public API
   ========================= */

void dht_setup(uint8_t dhtPin);
void dht_loop();

/* Blocking read (can block ~2s if sensor missing); avoid calling from main loop. */
bool sensorRead(SensorReadings &out);

/* Non-blocking: returns last reading from background task, or NaN and false if none yet / invalid. */
bool getLastSensorReadings(SensorReadings &out);

/* Fill out with NaN (no sensor); use when getLastSensorReadings returns false. */
void sensorReadingsInvalid(SensorReadings &out);

/* =========================
   Utilities (shared)
   ========================= */

float cToF(float c);
float absoluteHumidity_gm3(float tempC, float rh);
float dewPointC(float tempC, float rh);
