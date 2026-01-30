#pragma once

#include <Arduino.h>
#include "appstate_module.h"

/*
  Egg Profile Data
  ----------------
  Represents biological defaults for a species.
  These are READ-ONLY presets.
*/

struct EggProfileData {
  uint8_t id;
  const char *name;

  // === Incubation ===
  float incTempMinF;
  float incTempMaxF;
  float incHumMin;
  float incHumMax;
  uint16_t incTotalDays;
  uint8_t incTurnsPerDay;

  // === Holding / Preservation ===
  float holdTempMinF;
  float holdTempMaxF;
  float holdHumMin;
  float holdHumMax;
  uint16_t holdMaxDays;
  uint8_t holdTurnsPerDay;
};

extern const EggProfileData EGG_PROFILES[];
extern const uint8_t EGG_PROFILE_COUNT;

const EggProfileData *getProfileById(uint8_t id);

/* Module lifecycle */
void profiles_setup();
void profiles_loop();
