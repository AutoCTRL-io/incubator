#include "stepper_module.h"
#include <time.h>

static StepperConfig config;
static bool initialized = false;
static uint32_t absolutePosition = 0; // Total steps taken (0 = 0°, stepsPerTurn = 360°)
static time_t lastTurnEpoch = 0;
static uint32_t currentTurnsPerDay = 0;

void stepper_setup(const StepperConfig &cfg)
{
  config = cfg;

  pinMode(config.pinStep, OUTPUT);
  pinMode(config.pinDir, OUTPUT);
  pinMode(config.pinEnable, OUTPUT);

  digitalWrite(config.pinEnable, HIGH); // disabled by default
  digitalWrite(config.pinDir, config.invertDir ? HIGH : LOW);

  absolutePosition = 0;
  lastTurnEpoch = 0;
  currentTurnsPerDay = 0;
  initialized = true;
}

void stepper_loop()
{
  /* No periodic work; turning is triggered by core_module. */
}

void stepperEnable(bool enable)
{
  if (!initialized) return;
  digitalWrite(config.pinEnable, enable ? LOW : HIGH);
}

void stepperTurnOnce()
{
  if (!initialized) return;

  stepperEnable(true);

  for (uint32_t i = 0; i < config.stepsPerTurn; i++) {
    digitalWrite(config.pinStep, HIGH);
    delayMicroseconds(800);
    digitalWrite(config.pinStep, LOW);
    delayMicroseconds(800);
  }

  stepperEnable(false);

  // Update position (one full rotation = 360 degrees)
  absolutePosition += config.stepsPerTurn;

  // Update last turn time
  time_t now = time(nullptr);
  if (now >= 100000) {
    lastTurnEpoch = now;
  } else {
    lastTurnEpoch = (time_t)(millis() / 1000);
  }
}

MotorStatus stepperGetStatus()
{
  MotorStatus status;

  if (!initialized) {
    status.absolutePosition = 0;
    status.rotationPhase = 0.0f;
    status.lastTurnEpoch = 0;
    status.turnsPerDay = 0;
    status.secondsUntilNextTurn = 0;
    return status;
  }

  status.absolutePosition = absolutePosition;

  // Calculate phase: position modulo one full rotation, converted to degrees
  uint32_t positionInCurrentRotation = absolutePosition % config.stepsPerTurn;
  status.rotationPhase = (float)positionInCurrentRotation * 360.0f / (float)config.stepsPerTurn;

  status.lastTurnEpoch = lastTurnEpoch;
  status.turnsPerDay = currentTurnsPerDay;

  // Calculate seconds until next turn
  if (currentTurnsPerDay > 0 && lastTurnEpoch > 0) {
    time_t now = time(nullptr);
    if (now < 100000) {
      now = (time_t)(millis() / 1000);
    }

    uint32_t intervalSec = 86400UL / currentTurnsPerDay;
    uint32_t elapsed = (uint32_t)(now - lastTurnEpoch);

    if (elapsed < intervalSec) {
      status.secondsUntilNextTurn = intervalSec - elapsed;
    } else {
      status.secondsUntilNextTurn = 0; // Overdue
    }
  } else {
    status.secondsUntilNextTurn = 0;
  }

  return status;
}

void stepperSetTurnsPerDay(uint32_t turns)
{
  currentTurnsPerDay = turns;
}
