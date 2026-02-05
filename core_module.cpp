#include "core_module.h"
#include "appstate_module.h"
#include "climate_module.h"
#include "turning_module.h"
#include <time.h>
#include <math.h>

static uint8_t s_lampPin = 0;
static uint8_t s_humidifierPin = 0;
static uint8_t s_tempAlarmPin = 0;
static uint8_t s_humidityAlarmPin = 0;

/* Alarm thresholds: only assert alarms when outside range by more than this margin. */
static const float TEMP_ALARM_MARGIN_F = 0.5f;
static const float HUM_ALARM_MARGIN_RH = 5.0f;

/** Resolved inputs for the output-control mechanism. Mode/profile are resolved elsewhere; this is just "here are the ranges and flags, go." */
struct ResolvedTargets {
  float tminF;
  float tmaxF;
  float hmin;
  float hmax;
  bool outputsEnabled;   /* If false, lamp and humidifier stay off. */
  bool turningEnabled;
  uint16_t turningIntervalHours;
};

static ResolvedTargets s_lastResolved = { NAN, NAN, NAN, NAN, false, false, 0 };

static void applyLampAndHumidifier()
{
  if (s_lampPin)
    digitalWrite(s_lampPin, appstate_getLamp() ? LOW : HIGH);  /* Lamp on = LOW. */
  if (s_humidifierPin)
    digitalWrite(s_humidifierPin, appstate_getHumidifier() ? LOW : HIGH);  /* Humidifier on = LOW. */
}

/** Apply alarm GPIO outputs based on resolved targets and current sensor reading.
 *  Alarm outputs are active HIGH. When sensor/targets are invalid, alarms are deasserted. */
static void applyAlarms(const ResolvedTargets &resolved, const SensorReadings &sensor)
{
  if (!s_tempAlarmPin && !s_humidityAlarmPin)
    return;

  const bool sensorValid = !isnan(sensor.tempF) && !isnan(sensor.humidity);
  const bool targetsValid = !isnan(resolved.tminF) && !isnan(resolved.tmaxF) && !isnan(resolved.hmin) && !isnan(resolved.hmax);
  if (!sensorValid || !targetsValid) {
    if (s_tempAlarmPin) digitalWrite(s_tempAlarmPin, LOW);
    if (s_humidityAlarmPin) digitalWrite(s_humidityAlarmPin, LOW);
    return;
  }

  const bool tempAlarm =
    (sensor.tempF < (resolved.tminF - TEMP_ALARM_MARGIN_F)) ||
    (sensor.tempF > (resolved.tmaxF + TEMP_ALARM_MARGIN_F));
  const bool humAlarm =
    (sensor.humidity < (resolved.hmin - HUM_ALARM_MARGIN_RH)) ||
    (sensor.humidity > (resolved.hmax + HUM_ALARM_MARGIN_RH));

  if (s_tempAlarmPin) digitalWrite(s_tempAlarmPin, tempAlarm ? HIGH : LOW);
  if (s_humidityAlarmPin) digitalWrite(s_humidityAlarmPin, humAlarm ? HIGH : LOW);
}

/** Resolve current temp/humidity ranges and turning config from process (phase) or manual. Caller uses result to feed climate + turning only. */
static void resolveCurrentTargets(ResolvedTargets *out)
{
  if (process.active) {
    const IncubationPhase *phase = getActivePhase();
    out->tminF = phase->tempMinF;
    out->tmaxF = phase->tempMaxF;
    out->hmin = phase->humMin;
    out->hmax = phase->humMax;
    out->outputsEnabled = appstate_getSystemEnabled();
    out->turningEnabled = phase->turningEnabled;
    out->turningIntervalHours = phase->turnIntervalHours;
  } else {
    out->tminF = appstate_getManualTminF();
    out->tmaxF = appstate_getManualTmaxF();
    out->hmin = appstate_getManualHmin();
    out->hmax = appstate_getManualHmax();
    out->outputsEnabled = true;  /* Manual mode: allow lamp/humidifier from manual ranges. */
    out->turningEnabled = appstate_getManualTurningEnabled();
    out->turningIntervalHours = appstate_getManualTurnIntervalHours();
  }
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

/* Fallback egg-holding phase when profile has no holdingPhase (cool temp, turn once per day). */
static const IncubationPhase EGG_HOLDING_PHASE_DEFAULT = { 0, 0, 55.0f, 65.0f, 40.0f, 55.0f, true, 24 };

/* Resolve active phase by currentDay (or by process type for egg holding); updates process.activePhaseIndex. */
const IncubationPhase *getActivePhase()
{
  /* Egg holding: use profile's holdingPhase if set, else default (cool temp, no turning). */
  if (process.processType == PROCESS_EGG_HOLDING) {
    const EggProfileData *p = getProfileById(process.profileId);
    if (p && p->holdingPhase)
      return p->holdingPhase;
    return &EGG_HOLDING_PHASE_DEFAULT;
  }

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
  /* Prime resolved targets so status/UI has ranges before first coreUpdate. */
  resolveCurrentTargets(&s_lastResolved);
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

void core_setTempAlarmPin(uint8_t pin)
{
  s_tempAlarmPin = pin;
  if (pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);  /* Alarm off = LOW (active HIGH). */
  }
}

void core_setHumidityAlarmPin(uint8_t pin)
{
  s_humidityAlarmPin = pin;
  if (pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);  /* Alarm off = LOW (active HIGH). */
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
  /* Process lifecycle: when active, advance day and end run if past total days. */
  if (process.active) {
    process.currentDay = computeCurrentDay();
    uint16_t totalDays = 0;
    if (process.profileId == PROFILE_CUSTOM) {
      if (process.customPhaseCount > 0)
        totalDays = process.customPhases[process.customPhaseCount - 1].endDay;
    } else {
      const EggProfileData *p = getProfileById(process.profileId);
      if (p) totalDays = p->totalDays;
    }
    if (totalDays > 0 && process.currentDay >= totalDays) {
      process.active = false;
      process.processType = PROCESS_NONE;
      saveProcessState();
      appstate_setLamp(false);
      appstate_setHumidifier(false);
      applyLampAndHumidifier();
      /* End of run: deassert alarms. */
      applyAlarms(s_lastResolved, sensor);
      turning_configure(false, 0);
      return;
    }
    updateTargets();
  }

  /* Resolve ranges and flags from process (phase) or manual. Mode/profile handled here only. */
  ResolvedTargets resolved;
  resolveCurrentTargets(&resolved);
  s_lastResolved = resolved;

  turning_configure(resolved.turningEnabled, resolved.turningEnabled ? resolved.turningIntervalHours : (uint16_t)0);

  if (!resolved.outputsEnabled || (process.active && process.controlMode != CONTROL_MANAGED)) {
    appstate_setLamp(false);
    appstate_setHumidifier(false);
    applyLampAndHumidifier();
    /* Outputs disabled: deassert alarms (do not alarm when system is disabled). */
    if (s_tempAlarmPin) digitalWrite(s_tempAlarmPin, LOW);
    if (s_humidityAlarmPin) digitalWrite(s_humidityAlarmPin, LOW);
    return;
  }

  /* Single path: feed ranges and data into climate; apply outputs. No mode logic below. */
  climate_setTargets(resolved.tminF, resolved.tmaxF, resolved.hmin, resolved.hmax);
  climate_update(sensor);
  appstate_setLamp(climate_getLampOn());
  appstate_setHumidifier(climate_getHumidifierOn());
  applyLampAndHumidifier();
  applyAlarms(resolved, sensor);
}

/* Return the resolved ranges (phase or manual) so status/UI shows what is actually in use. */
float getActiveTargetMinF() { return s_lastResolved.tminF; }
float getActiveTargetMaxF() { return s_lastResolved.tmaxF; }
float getActiveHumMin()     { return s_lastResolved.hmin; }
float getActiveHumMax()     { return s_lastResolved.hmax; }

void markEggsTurned()
{
  time_t now = time(nullptr);
  if (now >= 100000) {
    process.lastTurnEpoch = now;
    saveProcessState();
  }
}
