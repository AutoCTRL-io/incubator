#pragma once

#include <Arduino.h>
#include "app_state.h"
#include "profiles.h"
#include "sensor_dht.h"
#include "motor_stepper.h"

void coreInit();
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
