#include "sensor_dht.h"
#include <DHT.h>
#include <math.h>

static DHT *dht = nullptr;
static uint8_t dhtPinInternal = 0;

/* =========================
   Initialization
   ========================= */

void sensorInit(uint8_t dhtPin)
{
  dhtPinInternal = dhtPin;

  if (dht)
    delete dht;

  dht = new DHT(dhtPinInternal, DHT22);
  dht->begin();
}

/* =========================
   Read + Validate Sensor
   ========================= */

bool sensorRead(SensorReadings &out)
{
  if (!dht)
    return false;

  float tC = dht->readTemperature();
  delay(50);
  float rh = dht->readHumidity();

  if (isnan(tC) || isnan(rh))
    return false;

  if (tC < -50.0f || tC > 100.0f || rh < 0.0f || rh > 100.0f)
    return false;

  out.tempC = tC;
  out.tempF = cToF(tC);
  out.humidity = rh;

  out.absHumidity = absoluteHumidity_gm3(tC, rh);

  out.dewPointC = dewPointC(tC, rh);
  out.dewPointF = isnan(out.dewPointC) ? NAN : cToF(out.dewPointC);

  out.heatIndexF = dht->computeHeatIndex(out.tempF, rh, true);
  out.heatIndexC = isnan(out.heatIndexF)
                       ? NAN
                       : (out.heatIndexF - 32.0f) * 5.0f / 9.0f;

  return true;
}

/* =========================
   Utilities
   ========================= */

float cToF(float c)
{
  return (c * 9.0f / 5.0f) + 32.0f;
}

// AH = 216.7 * ( (RH/100) * 6.112 * exp(17.67*T/(T+243.5)) ) / (T+273.15)
float absoluteHumidity_gm3(float tempC, float rh)
{
  float es = 6.112f * expf((17.67f * tempC) / (tempC + 243.5f));
  float e = (rh / 100.0f) * es;
  return 216.7f * (e / (tempC + 273.15f));
}

// Magnus formula
float dewPointC(float tempC, float rh)
{
  if (rh <= 0.0f)
    return NAN;

  const float a = 17.62f;
  const float b = 243.12f;

  float gamma = logf(rh / 100.0f) + (a * tempC) / (b + tempC);
  return (b * gamma) / (a - gamma);
}
