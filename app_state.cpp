#include "app_state.h"
#include <Preferences.h>

/* =========================
   Global Runtime State
   ========================= */

ProcessState process = {
  .active = false,

  .controlMode = CONTROL_UNMANAGED,
  .processType = PROCESS_NONE,

  .profileId = PROFILE_CHICKEN,

  .startEpoch = 0,
  .startDay = 1,
  .currentDay = 0,

  .lastTurnEpoch = 0,

  .customMinF = NAN,
  .customMaxF = NAN,
  .customHumMin = NAN,
  .customHumMax = NAN,
  .customTotalDays = 0,
  .customTurnsPerDay = 0
};

/* =========================
   Global Targets (active)
   ========================= */

float targetMinF = 98.0f;
float targetMaxF = 100.5f;
float targetHMin = 40.0f;
float targetHMax = 60.0f;

EggProfileId currentProfile = PROFILE_CHICKEN;

/* =========================
   System Flags
   ========================= */
// Note: otaInProgress is defined in ota_manager.cpp

/* =========================
   NVS Persistence
   ========================= */

static Preferences prefs;
static const char* NVS_NAMESPACE = "incubator";

void loadProcessState()
{
  prefs.begin(NVS_NAMESPACE, true); // read-only

  if (!prefs.getBool("valid", false)) {
    prefs.end();
    return; // No saved state
  }

  process.active = prefs.getBool("active", false);
  process.controlMode = (ControlMode)prefs.getUChar("controlMode", CONTROL_UNMANAGED);
  process.processType = (ProcessType)prefs.getUChar("processType", PROCESS_NONE);
  
  uint8_t loadedProfileId = prefs.getUChar("profileId", PROFILE_CHICKEN);
  // Validate profileId is within valid range
  if (loadedProfileId <= PROFILE_CUSTOM) {
    process.profileId = loadedProfileId;
  } else {
    process.profileId = PROFILE_CHICKEN; // Default to Chicken if invalid
  }

  process.startEpoch = (time_t)prefs.getULong64("startEpoch", 0);
  process.startDay = prefs.getUShort("startDay", 1);
  process.currentDay = prefs.getUShort("currentDay", 0);

  process.lastTurnEpoch = (time_t)prefs.getULong64("lastTurnEpoch", 0);

  process.customMinF = prefs.getFloat("customMinF", NAN);
  process.customMaxF = prefs.getFloat("customMaxF", NAN);
  process.customHumMin = prefs.getFloat("customHumMin", NAN);
  process.customHumMax = prefs.getFloat("customHumMax", NAN);
  process.customTotalDays = prefs.getUShort("customTotalDays", 0);
  process.customTurnsPerDay = prefs.getUChar("customTurnsPerDay", 0);

  prefs.end();
}

void saveProcessState()
{
  prefs.begin(NVS_NAMESPACE, false); // read-write

  prefs.putBool("valid", true);

  prefs.putBool("active", process.active);
  prefs.putUChar("controlMode", (uint8_t)process.controlMode);
  prefs.putUChar("processType", (uint8_t)process.processType);
  prefs.putUChar("profileId", process.profileId);

  prefs.putULong64("startEpoch", (uint64_t)process.startEpoch);
  prefs.putUShort("startDay", process.startDay);
  prefs.putUShort("currentDay", process.currentDay);

  prefs.putULong64("lastTurnEpoch", (uint64_t)process.lastTurnEpoch);

  prefs.putFloat("customMinF", process.customMinF);
  prefs.putFloat("customMaxF", process.customMaxF);
  prefs.putFloat("customHumMin", process.customHumMin);
  prefs.putFloat("customHumMax", process.customHumMax);
  prefs.putUShort("customTotalDays", process.customTotalDays);
  prefs.putUChar("customTurnsPerDay", process.customTurnsPerDay);

  prefs.end();
}

/* =========================
   Helpers
   ========================= */

void resetProcessState()
{
  process.active = false;
  process.controlMode = CONTROL_UNMANAGED;
  process.processType = PROCESS_NONE;
  process.profileId = PROFILE_CHICKEN;

  process.startEpoch = 0;
  process.startDay = 1;
  process.currentDay = 0;
  process.lastTurnEpoch = 0;

  process.customMinF = NAN;
  process.customMaxF = NAN;
  process.customHumMin = NAN;
  process.customHumMax = NAN;
  process.customTotalDays = 0;
  process.customTurnsPerDay = 0;

  saveProcessState();
}

bool isCustomProfileActive()
{
  return process.profileId == PROFILE_CUSTOM;
}

bool isProcessRunning()
{
  return process.active && process.processType != PROCESS_NONE;
}
