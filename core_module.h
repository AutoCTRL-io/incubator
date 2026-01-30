#pragma once

#include <Arduino.h>
#include "appstate_module.h"
#include "profiles_module.h"
#include "dht_module.h"
#include "stepper_module.h"

void core_setup();
void core_loop();

void coreUpdate(const SensorReadings &sensor);

// Process lifecycle
bool startProcess(ProcessType type, uint8_t profileId, uint16_t startDay);
void cancelProcess();
bool transitionProcess();

// Target resolution
float getActiveTargetMinF();
float getActiveTargetMaxF();
float getActiveHumMin();
float getActiveHumMax();

// Egg turning
bool shouldTurnEggs();
void markEggsTurned();
