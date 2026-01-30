#pragma once

#include <Arduino.h>
#include <time.h>

/* =========================
   Control / Process Modes
   ========================= */

enum ControlMode : uint8_t {
  CONTROL_UNMANAGED = 0,
  CONTROL_MANAGED   = 1
};

enum ProcessType : uint8_t {
  PROCESS_NONE = 0,
  PROCESS_EGG_HOLDING,
  PROCESS_INCUBATION
};

/* =========================
   Egg Profile IDs
   ========================= */

enum EggProfileId : uint8_t {
  PROFILE_CHICKEN = 0,
  PROFILE_COCKATIEL,
  PROFILE_CORMORANT,
  PROFILE_CRANE,
  PROFILE_DUCK,
  PROFILE_DUCK_MUSCOVY,
  PROFILE_EAGLE,
  PROFILE_EMU,
  PROFILE_FALCON,
  PROFILE_FLAMINGO,
  PROFILE_GOOSE,
  PROFILE_GROUSE,
  PROFILE_GUINEA_FOWL,
  PROFILE_HAWK,
  PROFILE_HERON,
  PROFILE_HUMMINGBIRD,
  PROFILE_LARGE_PARROTS,
  PROFILE_LOVEBIRD,
  PROFILE_OSTRICH,
  PROFILE_OWL,
  PROFILE_PARAKEET,
  PROFILE_PARROTS,
  PROFILE_PARTRIDGE,
  PROFILE_PEACOCK,
  PROFILE_PELICAN,
  PROFILE_PENGUIN,
  PROFILE_PHEASANT,
  PROFILE_PIGEON,
  PROFILE_QUAIL,
  PROFILE_RAIL,
  PROFILE_RHEA,
  PROFILE_SEABIRDS,
  PROFILE_SONGBIRDS,
  PROFILE_STORK,
  PROFILE_SWAN,
  PROFILE_TOUCAN,
  PROFILE_TURKEY,
  PROFILE_VULTURE,
  PROFILE_CUSTOM
};

/* =========================
   Static Egg Profile Data
   ========================= */
// Note: EggProfileData is defined in profiles_module.h

/* =========================
   Runtime Process State
   ========================= */

struct ProcessState {
  bool active;

  ControlMode controlMode;
  ProcessType processType;

  uint8_t profileId;

  time_t startEpoch;
  uint16_t startDay;
  uint16_t currentDay;

  time_t lastTurnEpoch;

  // Custom overrides
  float customMinF;
  float customMaxF;
  float customHumMin;
  float customHumMax;
  uint16_t customTotalDays;
  uint8_t customTurnsPerDay;
};

extern ProcessState process;

/* =========================
   Global Targets (active)
   ========================= */

extern float targetMinF;
extern float targetMaxF;
extern float targetHMin;
extern float targetHMax;

extern EggProfileId currentProfile;

/* =========================
   System Flags
   ========================= */

extern volatile bool otaInProgress;

/* =========================
   Functions
   ========================= */

void loadProcessState();
void saveProcessState();
void resetProcessState();
bool isCustomProfileActive();
bool isProcessRunning();

/* Module lifecycle */
void appstate_setup();
void appstate_loop();
