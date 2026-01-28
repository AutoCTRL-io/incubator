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

void sensorInit(uint8_t dhtPin);
bool sensorRead(SensorReadings &out);

/* =========================
   Utilities (shared)
   ========================= */

float cToF(float c);
float absoluteHumidity_gm3(float tempC, float rh);
float dewPointC(float tempC, float rh);
