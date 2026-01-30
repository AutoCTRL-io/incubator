#include "core_module.h"
#include <time.h>

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

static void updateTargets()
{
  if (isCustomProfileActive()) {
    targetMinF = process.customMinF;
    targetMaxF = process.customMaxF;
    targetHMin = process.customHumMin;
    targetHMax = process.customHumMax;
    return;
  }

  const EggProfileData *p = getProfileById(process.profileId);
  if (!p) return;

  if (process.processType == PROCESS_INCUBATION) {
    targetMinF = p->incTempMinF;
    targetMaxF = p->incTempMaxF;
    targetHMin = p->incHumMin;
    targetHMax = p->incHumMax;
  } else if (process.processType == PROCESS_EGG_HOLDING) {
    targetMinF = p->holdTempMinF;
    targetMaxF = p->holdTempMaxF;
    targetHMin = p->holdHumMin;
    targetHMax = p->holdHumMax;
  }
}

void core_setup()
{
  process.currentDay = 0;
  updateTargets();
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

  if (profileId == PROFILE_CUSTOM && process.customTurnsPerDay > 0) {
    stepperSetTurnsPerDay(process.customTurnsPerDay);
  } else {
    uint8_t turns = (type == PROCESS_INCUBATION) ? p->incTurnsPerDay : p->holdTurnsPerDay;
    stepperSetTurnsPerDay(turns);
  }

  updateTargets();
  saveProcessState();
  return true;
}

void cancelProcess()
{
  process.active = false;
  process.controlMode = CONTROL_UNMANAGED;
  process.processType = PROCESS_NONE;
  process.startEpoch = 0;
  process.startDay = 1;
  process.currentDay = 0;
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

  const EggProfileData *p = getProfileById(process.profileId);
  if (p) {
    stepperSetTurnsPerDay(p->incTurnsPerDay);
  }

  updateTargets();
  saveProcessState();
  return true;
}

void coreUpdate(const SensorReadings &sensor)
{
  (void)sensor;

  if (!process.active)
    return;

  process.currentDay = computeCurrentDay();

  const EggProfileData *p = getProfileById(process.profileId);
  if (p && process.processType == PROCESS_INCUBATION) {
    uint16_t totalDays = isCustomProfileActive() ? process.customTotalDays : p->incTotalDays;
    if (totalDays > 0 && process.currentDay >= totalDays) {
      process.active = false;
      process.processType = PROCESS_NONE;
      saveProcessState();
      return;
    }
  }

  if (shouldTurnEggs()) {
    stepperTurnOnce();
    markEggsTurned();

    if (isCustomProfileActive() && process.customTurnsPerDay > 0) {
      stepperSetTurnsPerDay(process.customTurnsPerDay);
    }
  }
}

float getActiveTargetMinF()
{
  if (isCustomProfileActive())
    return process.customMinF;

  const EggProfileData *p = getProfileById(process.profileId);
  if (!p) return NAN;

  return (process.processType == PROCESS_INCUBATION)
           ? p->incTempMinF
           : p->holdTempMinF;
}

float getActiveTargetMaxF()
{
  if (isCustomProfileActive())
    return process.customMaxF;

  const EggProfileData *p = getProfileById(process.profileId);
  if (!p) return NAN;

  return (process.processType == PROCESS_INCUBATION)
           ? p->incTempMaxF
           : p->holdTempMaxF;
}

float getActiveHumMin()
{
  if (isCustomProfileActive())
    return process.customHumMin;

  const EggProfileData *p = getProfileById(process.profileId);
  if (!p) return NAN;

  return (process.processType == PROCESS_INCUBATION)
           ? p->incHumMin
           : p->holdHumMin;
}

float getActiveHumMax()
{
  if (isCustomProfileActive())
    return process.customHumMax;

  const EggProfileData *p = getProfileById(process.profileId);
  if (!p) return NAN;

  return (process.processType == PROCESS_INCUBATION)
           ? p->incHumMax
           : p->holdHumMax;
}

bool shouldTurnEggs()
{
  if (!process.active)
    return false;

  const EggProfileData *p = getProfileById(process.profileId);
  if (!p)
    return false;

  uint8_t turns = 0;
  if (isCustomProfileActive()) {
    turns = process.customTurnsPerDay;
  } else {
    turns = (process.processType == PROCESS_INCUBATION)
              ? p->incTurnsPerDay
              : p->holdTurnsPerDay;
  }

  if (turns == 0)
    return false;

  time_t now = time(nullptr);
  if (now < 100000) return false;

  if (process.lastTurnEpoch == 0)
    return true;

  uint32_t intervalSec = 86400UL / turns;
  return (now - process.lastTurnEpoch) >= intervalSec;
}

void markEggsTurned()
{
  time_t now = time(nullptr);
  if (now >= 100000) {
    process.lastTurnEpoch = now;
    saveProcessState();
  }
}
