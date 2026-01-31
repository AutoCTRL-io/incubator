/*
  Stepper stub: no motor hardware; unused until real stepper is added.
  Core no longer references this module. When adding real stepper: read
  app state in stepper_loop(), decide when to turn, run motor, call
  markEggsTurned() (from core) after turning—invoke from main loop only.
*/
#include "stepper_module.h"
#include <time.h>

static StepperConfig config;
static bool initialized = false;
static uint32_t absolutePosition = 0;
static time_t lastTurnEpoch = 0;
static uint32_t currentTurnsPerDay = 0;

void stepper_setup(const StepperConfig &cfg)
{
  config = cfg;
  absolutePosition = 0;
  lastTurnEpoch = 0;
  currentTurnsPerDay = 0;
  initialized = true;
}

void stepper_loop()
{
}

void stepperEnable(bool enable)
{
  (void)enable;
}

void stepperTurnOnce()
{
  if (!initialized) return;
  absolutePosition += config.stepsPerTurn;
  time_t now = time(nullptr);
  lastTurnEpoch = (now >= 100000) ? now : (time_t)(millis() / 1000);
}

MotorStatus stepperGetStatus()
{
  MotorStatus status = { 0, 0.0f, 0, 0, 0 };
  if (!initialized) return status;
  status.absolutePosition = absolutePosition;
  status.rotationPhase = (float)(absolutePosition % config.stepsPerTurn) * 360.0f / (float)config.stepsPerTurn;
  status.lastTurnEpoch = lastTurnEpoch;
  status.turnsPerDay = currentTurnsPerDay;
  if (currentTurnsPerDay > 0 && lastTurnEpoch > 0) {
    time_t now = time(nullptr);
    if (now < 100000) now = (time_t)(millis() / 1000);
    uint32_t intervalSec = 86400UL / currentTurnsPerDay;
    uint32_t elapsed = (uint32_t)(now - lastTurnEpoch);
    status.secondsUntilNextTurn = (elapsed < intervalSec) ? (intervalSec - elapsed) : 0;
  }
  return status;
}

void stepperSetTurnsPerDay(uint32_t turns)
{
  currentTurnsPerDay = turns;
}
