#pragma once

#include <Arduino.h>
#include <time.h>

/*
  Stepper Motor Controller
  ------------------------
  Responsible for:
  - Initializing motor pins
  - Rotating eggs by a fixed step count
  - Tracking absolute position and rotation phase
  - Abstracting hardware from core logic

  This module does NOT decide *when* to turn eggs.
*/

struct StepperConfig {
  uint8_t pinStep;
  uint8_t pinDir;
  uint8_t pinEnable;
  uint32_t stepsPerTurn;
  bool invertDir;
};

// Motor status information
struct MotorStatus {
  uint32_t absolutePosition;  // Total steps since init (0-360 degrees = 0-360)
  float rotationPhase;         // Current phase in degrees (0.0-360.0)
  time_t lastTurnEpoch;        // When last turn completed
  uint32_t turnsPerDay;        // Current target turns per day
  uint32_t secondsUntilNextTurn; // Time until next scheduled turn
};

void stepper_setup(const StepperConfig &cfg);
void stepper_loop();

void stepperEnable(bool enable);
void stepperTurnOnce();
MotorStatus stepperGetStatus();
void stepperSetTurnsPerDay(uint32_t turns);
