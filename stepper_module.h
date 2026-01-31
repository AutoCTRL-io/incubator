#pragma once

#include <Arduino.h>
#include <time.h>

/*
  Stepper Motor Controller
  ------------------------
  Design (for real implementation):
  - Stepper should own ALL code that checks the time and turns the motor.
  - Call stepper from the main loop (preferred), or from core—not both.
  - Best: main loop calls stepper; stepper reads app state directly and
    decides when to run (if at all). Less coupling = more flexibility.
  - Responsible for: init pins, rotate by step count, track position/phase,
    and (when implemented) reading app state to decide when to turn.
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
