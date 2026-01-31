#pragma once

#include <Arduino.h>
#include "appstate_module.h"
#include "profiles_module.h"
#include "dht_module.h"

void core_setup();
void core_loop();
/* Relay drives the lamp; core decides lamp and drives relay directly. Call after core_setup(). */
void core_setRelayPin(uint8_t pin);

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

// Stepper (or other) calls this after turning eggs to update app state.
void markEggsTurned();
