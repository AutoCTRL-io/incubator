/*
  Phase-based egg profiles.
  Day counter is 0-based: day 0 = first day. All phases use startDay/endDay inclusive (0 to totalDays-1).
  Chicken has 2 phases (incubation + lockdown); others have 1 phase each.
  Custom (38) has no static phases; uses ProcessState.customPhases.
  holdingPhase: optional egg-holding settings (cool temp, no turning) per profile; null = use core default.
*/

#include "profiles_module.h"
#include <math.h>

/* Shared default egg-holding phase (cool temp, turn once per day). Used when a profile has no holdingPhase. */
static const IncubationPhase defaultHoldingPhase = { 0, 0, 55.0f, 65.0f, 40.0f, 55.0f, true, 24 };

/* Chicken: days 0-17 turning every 2h; days 18-20 lockdown (no turning), higher humidity. */
static const IncubationPhase chickenPhases[] = {
  { 0, 17, 99.0f, 100.0f, 45.0f, 55.0f, true, 2 },
  { 18, 20, 99.0f, 100.0f, 65.0f, 75.0f, false, 0 }
};

/* Single-phase presets: days 0 to totalDays-1, turning every 2h. */
static const IncubationPhase cockatielPhases[] = { { 0, 17, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase cormorantPhases[] = { { 0, 27, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase cranePhases[] = { { 0, 29, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase duckPhases[] = { { 0, 27, 99.5f, 100.0f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase duckMuscovyPhases[] = { { 0, 34, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase eaglePhases[] = { { 0, 34, 99.0f, 99.5f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase emuPhases[] = { { 0, 49, 96.5f, 97.5f, 40.0f, 50.0f, true, 2 } };
static const IncubationPhase falconPhases[] = { { 0, 31, 99.0f, 99.5f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase flamingoPhases[] = { { 0, 27, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase goosePhases[] = { { 0, 29, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase grousePhases[] = { { 0, 23, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase guineaFowlPhases[] = { { 0, 25, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase hawkPhases[] = { { 0, 31, 99.0f, 99.5f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase heronPhases[] = { { 0, 27, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase hummingbirdPhases[] = { { 0, 13, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase largeParrotsPhases[] = { { 0, 25, 99.0f, 99.5f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase lovebirdPhases[] = { { 0, 22, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase ostrichPhases[] = { { 0, 41, 96.0f, 97.0f, 40.0f, 50.0f, true, 2 } };
static const IncubationPhase owlPhases[] = { { 0, 29, 99.0f, 99.5f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase parakeetPhases[] = { { 0, 17, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase parrotsPhases[] = { { 0, 25, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase partridgePhases[] = { { 0, 23, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase peacockPhases[] = { { 0, 27, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase pelicanPhases[] = { { 0, 29, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase penguinPhases[] = { { 0, 34, 98.5f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase pheasantPhases[] = { { 0, 23, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase pigeonPhases[] = { { 0, 17, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase quailPhases[] = { { 0, 16, 99.5f, 100.5f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase railPhases[] = { { 0, 19, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase rheaPhases[] = { { 0, 39, 97.0f, 98.0f, 40.0f, 50.0f, true, 2 } };
static const IncubationPhase seabirdsPhases[] = { { 0, 27, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase songbirdsPhases[] = { { 0, 13, 99.5f, 100.0f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase storkPhases[] = { { 0, 29, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase swanPhases[] = { { 0, 34, 99.0f, 99.5f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase toucanPhases[] = { { 0, 17, 99.0f, 99.5f, 45.0f, 55.0f, true, 2 } };
static const IncubationPhase turkeyPhases[] = { { 0, 27, 99.0f, 100.0f, 50.0f, 60.0f, true, 2 } };
static const IncubationPhase vulturePhases[] = { { 0, 41, 99.0f, 99.5f, 45.0f, 55.0f, true, 2 } };

const EggProfileData EGG_PROFILES[] = {
  { PROFILE_CHICKEN, "Chicken", 21, chickenPhases, 2, &defaultHoldingPhase },
  { PROFILE_COCKATIEL, "Cockatiel", 18, cockatielPhases, 1, &defaultHoldingPhase },
  { PROFILE_CORMORANT, "Cormorant", 28, cormorantPhases, 1, &defaultHoldingPhase },
  { PROFILE_CRANE, "Crane", 30, cranePhases, 1, &defaultHoldingPhase },
  { PROFILE_DUCK, "Duck", 28, duckPhases, 1, &defaultHoldingPhase },
  { PROFILE_DUCK_MUSCOVY, "Duck Muscovy", 35, duckMuscovyPhases, 1, &defaultHoldingPhase },
  { PROFILE_EAGLE, "Eagle", 35, eaglePhases, 1, &defaultHoldingPhase },
  { PROFILE_EMU, "Emu", 50, emuPhases, 1, &defaultHoldingPhase },
  { PROFILE_FALCON, "Falcon", 32, falconPhases, 1, &defaultHoldingPhase },
  { PROFILE_FLAMINGO, "Flamingo", 28, flamingoPhases, 1, &defaultHoldingPhase },
  { PROFILE_GOOSE, "Goose", 30, goosePhases, 1, &defaultHoldingPhase },
  { PROFILE_GROUSE, "Grouse", 24, grousePhases, 1, &defaultHoldingPhase },
  { PROFILE_GUINEA_FOWL, "Guinea Fowl", 26, guineaFowlPhases, 1, &defaultHoldingPhase },
  { PROFILE_HAWK, "Hawk", 32, hawkPhases, 1, &defaultHoldingPhase },
  { PROFILE_HERON, "Heron", 28, heronPhases, 1, &defaultHoldingPhase },
  { PROFILE_HUMMINGBIRD, "Hummingbird", 14, hummingbirdPhases, 1, &defaultHoldingPhase },
  { PROFILE_LARGE_PARROTS, "Large Parrots", 26, largeParrotsPhases, 1, &defaultHoldingPhase },
  { PROFILE_LOVEBIRD, "Lovebird", 23, lovebirdPhases, 1, &defaultHoldingPhase },
  { PROFILE_OSTRICH, "Ostrich", 42, ostrichPhases, 1, &defaultHoldingPhase },
  { PROFILE_OWL, "Owl", 30, owlPhases, 1, &defaultHoldingPhase },
  { PROFILE_PARAKEET, "Parakeet", 18, parakeetPhases, 1, &defaultHoldingPhase },
  { PROFILE_PARROTS, "Parrots", 26, parrotsPhases, 1, &defaultHoldingPhase },
  { PROFILE_PARTRIDGE, "Partridge", 24, partridgePhases, 1, &defaultHoldingPhase },
  { PROFILE_PEACOCK, "Peacock", 28, peacockPhases, 1, &defaultHoldingPhase },
  { PROFILE_PELICAN, "Pelican", 30, pelicanPhases, 1, &defaultHoldingPhase },
  { PROFILE_PENGUIN, "Penguin", 35, penguinPhases, 1, &defaultHoldingPhase },
  { PROFILE_PHEASANT, "Pheasant", 24, pheasantPhases, 1, &defaultHoldingPhase },
  { PROFILE_PIGEON, "Pigeon", 18, pigeonPhases, 1, &defaultHoldingPhase },
  { PROFILE_QUAIL, "Quail", 17, quailPhases, 1, &defaultHoldingPhase },
  { PROFILE_RAIL, "Rail", 20, railPhases, 1, &defaultHoldingPhase },
  { PROFILE_RHEA, "Rhea", 40, rheaPhases, 1, &defaultHoldingPhase },
  { PROFILE_SEABIRDS, "Seabirds", 28, seabirdsPhases, 1, &defaultHoldingPhase },
  { PROFILE_SONGBIRDS, "Songbirds", 14, songbirdsPhases, 1, &defaultHoldingPhase },
  { PROFILE_STORK, "Stork", 30, storkPhases, 1, &defaultHoldingPhase },
  { PROFILE_SWAN, "Swan", 35, swanPhases, 1, &defaultHoldingPhase },
  { PROFILE_TOUCAN, "Toucan", 18, toucanPhases, 1, &defaultHoldingPhase },
  { PROFILE_TURKEY, "Turkey", 28, turkeyPhases, 1, &defaultHoldingPhase },
  { PROFILE_VULTURE, "Vulture", 42, vulturePhases, 1, &defaultHoldingPhase },
  { PROFILE_CUSTOM, "Custom", 0, nullptr, 0, nullptr }
};

const uint8_t EGG_PROFILE_COUNT =
  sizeof(EGG_PROFILES) / sizeof(EGG_PROFILES[0]);

const EggProfileData *getProfileById(uint8_t id)
{
  for (uint8_t i = 0; i < EGG_PROFILE_COUNT; i++) {
    if (EGG_PROFILES[i].id == id)
      return &EGG_PROFILES[i];
  }
  return nullptr;
}

void profiles_setup()
{
}

void profiles_loop()
{
}
