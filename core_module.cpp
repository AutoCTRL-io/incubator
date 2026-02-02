#include "core_module.h"
#include "appstate_module.h"
#include "climate_module.h"
#include "turning_module.h"
#include <time.h>
#include <math.h>

static uint8_t s_lampPin = 0;
static uint8_t s_humidifierPin = 0;

static void applyLampAndHumidifier()
{
  if (s_lampPin)
    digitalWrite(s_lampPin, appstate_getLamp() ? LOW : HIGH);  /* Lamp on = LOW. */
  if (s_humidifierPin)
    digitalWrite(s_humidifierPin, appstate_getHumidifier() ? LOW : HIGH);  /* Humidifier on = LOW. */
}

static uint16_t computeCurrentDay()
{
  if (!process.startEpoch)
    return process.startDay;

  time_t now = time(nullptr);
  if (now < 100000) // Time not set yet
    return process.startDay;

  uint32_t elapsed = now - process.startEpoch;
  return process.startDay + (elapsed / 86400UL);
}

/* Default phase when Custom has no phases (targets NAN, turning off). */
static const IncubationPhase DEFAULT_PHASE = { 0, 0, NAN, NAN, NAN, NAN, false, 0 };

/* Resolve active phase by currentDay; updates process.activePhaseIndex. */
const IncubationPhase *getActivePhase()
{
  uint16_t day = process.currentDay;

  if (process.profileId == PROFILE_CUSTOM) {
    if (process.customPhaseCount == 0)
      return &DEFAULT_PHASE;
    for (uint8_t i = 0; i < process.customPhaseCount; i++) {
      if (day >= process.customPhases[i].startDay && day <= process.customPhases[i].endDay) {
        process.activePhaseIndex = i;
        return &process.customPhases[i];
      }
    }
    process.activePhaseIndex = process.customPhaseCount - 1;
    return &process.customPhases[process.activePhaseIndex];
  }

  const EggProfileData *p = getProfileById(process.profileId);
  if (!p || !p->phases || p->phaseCount == 0)
    return &DEFAULT_PHASE;

  for (uint8_t i = 0; i < p->phaseCount; i++) {
    if (day >= p->phases[i].startDay && day <= p->phases[i].endDay) {
      process.activePhaseIndex = i;
      return &p->phases[i];
    }
  }
  /* Day past last phase: use last phase. */
  process.activePhaseIndex = p->phaseCount - 1;
  return &p->phases[process.activePhaseIndex];
}

static void updateTargets()
{
  const IncubationPhase *phase = getActivePhase();
  targetMinF = phase->tempMinF;
  targetMaxF = phase->tempMaxF;
  targetHMin = phase->humMin;
  targetHMax = phase->humMax;
}

void core_setup()
{
  process.currentDay = 0;
  updateTargets();
}

void core_setLampPin(uint8_t pin)
{
  s_lampPin = pin;
  if (pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);  /* Lamp off = HIGH. */
  }
}

void core_setHumidifierPin(uint8_t pin)
{
  s_humidifierPin = pin;
  if (pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);  /* Humidifier off = HIGH. */
  }
}

void core_loop()
{
  /* coreUpdate() is called by main loop with sensor data when those modules are active. */
}

bool startProcess(ProcessType type, uint8_t profileId, uint16_t startDay)
{
  const EggProfileData *p = getProfileById(profileId);
  if (!p) return false;

  if (process.active) {
    return false; // Process already running
  }

  process.active = true;
  process.controlMode = CONTROL_MANAGED;
  process.processType = type;
  process.profileId = profileId;
  currentProfile = (EggProfileId)profileId;

  time_t now = time(nullptr);
  if (now < 100000) {
    process.startEpoch = (time_t)(millis() / 1000);
  } else {
    process.startEpoch = now;
  }
  process.startDay = startDay;
  process.currentDay = startDay;
  process.lastTurnEpoch = 0;
  process.activePhaseIndex = 0;

  updateTargets();
  turning_configure(getActivePhase()->turningEnabled, getActivePhase()->turnIntervalHours);
  saveProcessState();
  return true;
}

void cancelProcess()
{
  process.active = false;
  process.controlMode = CONTROL_UNMANAGED;
  process.processType = PROCESS_NONE;
  process.startEpoch = 0;
  process.startDay = 0;
  process.currentDay = 0;
  process.activePhaseIndex = 0;
  process.lastTurnEpoch = 0;
  currentProfile = PROFILE_CHICKEN;

  updateTargets();
  saveProcessState();
}

bool transitionProcess()
{
  if (!process.active) return false;
  if (process.processType != PROCESS_EGG_HOLDING) return false;

  process.processType = PROCESS_INCUBATION;

  updateTargets();
  saveProcessState();
  return true;
}

void coreUpdate(const SensorReadings &sensor)
{
  if (!process.active) {
    appstate_setLamp(false);
    appstate_setHumidifier(false);
    applyLampAndHumidifier();
    /* Manual mode: use user's tilting preference (saved via set_turning). */
    bool manualOn = appstate_getManualTurningEnabled();
    uint16_t manualInterval = appstate_getManualTurnIntervalHours();
    turning_configure(manualOn, manualOn ? manualInterval : (uint16_t)0);
    return;
  }

  process.currentDay = computeCurrentDay();

  /* End of run: totalDays from profile (preset) or last custom phase endDay. */
  uint16_t totalDays = 0;
  if (process.profileId == PROFILE_CUSTOM) {
    if (process.customPhaseCount > 0)
      totalDays = process.customPhases[process.customPhaseCount - 1].endDay;
  } else {
    const EggProfileData *p = getProfileById(process.profileId);
    if (p) totalDays = p->totalDays;
  }
  /* Days are 0-based (0 to totalDays-1); process ends when currentDay >= totalDays. */
  if (totalDays > 0 && process.currentDay >= totalDays) {
    process.active = false;
    process.processType = PROCESS_NONE;
    saveProcessState();
    appstate_setLamp(false);
    appstate_setHumidifier(false);
    applyLampAndHumidifier();
    turning_configure(false, 0);
    return;
  }

  /* Resolve active phase; tell climate and turning what to use. */
  const IncubationPhase *phase = getActivePhase();
  climate_setTargets(phase->tempMinF, phase->tempMaxF, phase->humMin, phase->humMax);
  turning_configure(phase->turningEnabled, phase->turnIntervalHours);
  updateTargets();

  /* When system is Off: no lamp or motor output. */
  if (!appstate_getSystemEnabled()) {
    appstate_setLamp(false);
    appstate_setHumidifier(false);
    applyLampAndHumidifier();
    turning_configure(false, 0);
    return;
  }

  if (process.controlMode != CONTROL_MANAGED) {
    appstate_setLamp(false);
    appstate_setHumidifier(false);
    applyLampAndHumidifier();
    turning_configure(false, 0);
    return;
  }

  /* Climate decides lamp and humidifier from targets and sensor. */
  climate_update(sensor);
  appstate_setLamp(climate_getLampOn());
  appstate_setHumidifier(climate_getHumidifierOn());
  applyLampAndHumidifier();
}

float getActiveTargetMinF()
{
  return getActivePhase()->tempMinF;
}

float getActiveTargetMaxF()
{
  return getActivePhase()->tempMaxF;
}

float getActiveHumMin()
{
  return getActivePhase()->humMin;
}

float getActiveHumMax()
{
  return getActivePhase()->humMax;
}

void markEggsTurned()
{
  time_t now = time(nullptr);
  if (now >= 100000) {
    process.lastTurnEpoch = now;
    saveProcessState();
  }
}
