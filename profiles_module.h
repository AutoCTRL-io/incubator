#pragma once

#include <Arduino.h>
#include "appstate_module.h"

/*
  Egg Profile Data (phase-based)
  ------------------------------
  Represents biological defaults for a species.
  READ-ONLY presets; each phase covers a day range with temp, humidity, turning.
*/

struct EggProfileData {
  uint8_t id;
  const char *name;

  uint16_t totalDays;

  const IncubationPhase *phases;
  uint8_t phaseCount;

  /* Optional egg-holding phase (cool temp, no turning). If null, core uses default. */
  const IncubationPhase *holdingPhase;
};

extern const EggProfileData EGG_PROFILES[];
extern const uint8_t EGG_PROFILE_COUNT;

const EggProfileData *getProfileById(uint8_t id);

/* Module lifecycle */
void profiles_setup();
void profiles_loop();
