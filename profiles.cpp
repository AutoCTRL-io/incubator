#include "profiles.h"
#include <math.h>

/*
  NOTE:
  Values are representative defaults.
  Temperature ranges from README.md.
  Humidity, days, and turns per day are reasonable defaults.
*/

const EggProfileData EGG_PROFILES[] = {
  // 0
  {
    PROFILE_CHICKEN, "Chicken",
    98.0f, 100.5f, 45.0f, 55.0f, 21, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 1
  {
    PROFILE_COCKATIEL, "Cockatiel",
    99.5f, 100.0f, 45.0f, 55.0f, 18, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 10, 2
  },
  // 2
  {
    PROFILE_CORMORANT, "Cormorant",
    99.0f, 99.5f, 50.0f, 60.0f, 28, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 3
  {
    PROFILE_CRANE, "Crane",
    99.0f, 99.5f, 50.0f, 60.0f, 30, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 4
  {
    PROFILE_DUCK, "Duck",
    99.5f, 100.0f, 50.0f, 60.0f, 28, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 5
  {
    PROFILE_DUCK_MUSCOVY, "Duck Muscovy",
    99.0f, 99.5f, 50.0f, 60.0f, 35, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 6
  {
    PROFILE_EAGLE, "Eagle",
    99.0f, 99.5f, 45.0f, 55.0f, 35, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 7
  {
    PROFILE_EMU, "Emu",
    96.5f, 97.5f, 40.0f, 50.0f, 50, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 8
  {
    PROFILE_FALCON, "Falcon",
    99.0f, 99.5f, 45.0f, 55.0f, 32, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 9
  {
    PROFILE_FLAMINGO, "Flamingo",
    99.0f, 99.5f, 50.0f, 60.0f, 28, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 10
  {
    PROFILE_GOOSE, "Goose",
    99.0f, 99.5f, 50.0f, 60.0f, 30, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 11
  {
    PROFILE_GROUSE, "Grouse",
    99.5f, 100.0f, 45.0f, 55.0f, 24, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 12
  {
    PROFILE_GUINEA_FOWL, "Guinea Fowl",
    99.5f, 100.0f, 45.0f, 55.0f, 26, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 13
  {
    PROFILE_HAWK, "Hawk",
    99.0f, 99.5f, 45.0f, 55.0f, 32, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 14
  {
    PROFILE_HERON, "Heron",
    99.0f, 99.5f, 50.0f, 60.0f, 28, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 15
  {
    PROFILE_HUMMINGBIRD, "Hummingbird",
    99.5f, 100.0f, 45.0f, 55.0f, 14, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 7, 2
  },
  // 16
  {
    PROFILE_LARGE_PARROTS, "Large Parrots",
    99.0f, 99.5f, 45.0f, 55.0f, 26, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 10, 2
  },
  // 17
  {
    PROFILE_LOVEBIRD, "Lovebird",
    99.5f, 100.0f, 45.0f, 55.0f, 23, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 10, 2
  },
  // 18
  {
    PROFILE_OSTRICH, "Ostrich",
    96.0f, 97.0f, 40.0f, 50.0f, 42, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 19
  {
    PROFILE_OWL, "Owl",
    99.0f, 99.5f, 45.0f, 55.0f, 30, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 20
  {
    PROFILE_PARAKEET, "Parakeet",
    99.5f, 100.0f, 45.0f, 55.0f, 18, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 10, 2
  },
  // 21
  {
    PROFILE_PARROTS, "Parrots",
    99.5f, 100.0f, 45.0f, 55.0f, 26, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 10, 2
  },
  // 22
  {
    PROFILE_PARTRIDGE, "Partridge",
    99.5f, 100.0f, 45.0f, 55.0f, 24, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 23
  {
    PROFILE_PEACOCK, "Peacock",
    99.5f, 100.0f, 45.0f, 55.0f, 28, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 24
  {
    PROFILE_PELICAN, "Pelican",
    99.0f, 99.5f, 50.0f, 60.0f, 30, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 25
  {
    PROFILE_PENGUIN, "Penguin",
    98.5f, 99.5f, 50.0f, 60.0f, 35, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 26
  {
    PROFILE_PHEASANT, "Pheasant",
    99.5f, 100.0f, 45.0f, 55.0f, 24, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 27
  {
    PROFILE_PIGEON, "Pigeon",
    99.5f, 100.0f, 45.0f, 55.0f, 18, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 10, 2
  },
  // 28
  {
    PROFILE_QUAIL, "Quail",
    99.5f, 100.5f, 45.0f, 55.0f, 17, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 10, 2
  },
  // 29
  {
    PROFILE_RAIL, "Rail",
    99.0f, 99.5f, 50.0f, 60.0f, 20, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 10, 2
  },
  // 30
  {
    PROFILE_RHEA, "Rhea",
    97.0f, 98.0f, 40.0f, 50.0f, 40, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 31
  {
    PROFILE_SEABIRDS, "Seabirds",
    99.0f, 99.5f, 50.0f, 60.0f, 28, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 32
  {
    PROFILE_SONGBIRDS, "Songbirds",
    99.5f, 100.0f, 45.0f, 55.0f, 14, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 7, 2
  },
  // 33
  {
    PROFILE_STORK, "Stork",
    99.0f, 99.5f, 50.0f, 60.0f, 30, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 34
  {
    PROFILE_SWAN, "Swan",
    99.0f, 99.5f, 50.0f, 60.0f, 35, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 35
  {
    PROFILE_TOUCAN, "Toucan",
    99.0f, 99.5f, 45.0f, 55.0f, 18, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 10, 2
  },
  // 36
  {
    PROFILE_TURKEY, "Turkey",
    99.0f, 100.0f, 50.0f, 60.0f, 28, 4,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 37
  {
    PROFILE_VULTURE, "Vulture",
    99.0f, 99.5f, 45.0f, 55.0f, 42, 3,
    55.0f, 65.0f, 70.0f, 80.0f, 14, 2
  },
  // 38
  {
    PROFILE_CUSTOM, "Custom",
    NAN, NAN, NAN, NAN, 0, 0,
    NAN, NAN, NAN, NAN, 0, 0
  }
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
