#pragma once

#include <Arduino.h>
#include "appstate_module.h"
#include "profiles_module.h"
#include "dht_module.h"

void core_setup();
void core_loop();
/* Lamp and humidifier GPIO; core applies climate decisions. Call after core_setup(). */
void core_setLampPin(uint8_t pin);
void core_setHumidifierPin(uint8_t pin);

void coreUpdate(const SensorReadings &sensor);

// Process lifecycle
bool startProcess(ProcessType type, uint8_t profileId, uint16_t startDay);
void cancelProcess();
bool transitionProcess();

// Phase resolution: returns current phase for process; updates process.activePhaseIndex.
const IncubationPhase *getActivePhase();

// Target resolution (from active phase)
float getActiveTargetMinF();
float getActiveTargetMaxF();
float getActiveHumMin();
float getActiveHumMax();

// Stepper (or other) calls this after turning eggs to update app state.
void markEggsTurned();
