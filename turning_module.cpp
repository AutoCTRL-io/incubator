/*
  Egg turning: config (enabled + interval hours) and timer. No DHT, no appstate.
  Core tells us enabled/interval; we run on a timer and call stepper + markEggsTurned.
*/

#include "turning_module.h"
#include "stepper_module.h"
#include "core_module.h"
#include <time.h>

static bool s_enabled = false;
static uint16_t s_intervalHours = 0;
static bool s_didTurnLastLoop = false;

void turning_configure(bool enabled, uint16_t intervalHours)
{
  s_enabled = enabled;
  s_intervalHours = intervalHours;
  if (!s_enabled || s_intervalHours == 0) {
    stepperSetTurnsPerDay(0);
    return;
  }
  stepperSetTurnsPerDay((uint32_t)(24 / s_intervalHours));
}

void turning_loop()
{
  if (!s_enabled || s_intervalHours == 0)
    return;

  MotorStatus st = stepperGetStatus();
  time_t now = time(nullptr);
  if (now < 100000)
    now = (time_t)(millis() / 1000);

  uint32_t intervalSec = (uint32_t)s_intervalHours * 3600UL;
  time_t lastTurn = st.lastTurnEpoch;
  if (lastTurn == 0) {
    /* First run: trigger immediately so we have a baseline. */
    stepperTurnOnce();
    markEggsTurned();
    s_didTurnLastLoop = true;
    return;
  }
  uint32_t elapsed = (uint32_t)(now - lastTurn);
  if (elapsed >= intervalSec) {
    stepperTurnOnce();
    markEggsTurned();
    s_didTurnLastLoop = true;
  }
}

void turning_tiltNow()
{
  stepperTurnOnce();
  markEggsTurned();
}

bool turning_didTurnLastLoop()
{
  bool v = s_didTurnLastLoop;
  s_didTurnLastLoop = false;
  return v;
}
