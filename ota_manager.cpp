#include "ota_manager.h"
#include <ArduinoOTA.h>

volatile bool otaInProgress = false;

void otaInit()
{
  ArduinoOTA
    .onStart([]() {
      otaInProgress = true;
    })
    .onEnd([]() {
      otaInProgress = false;
    })
    .onError([](ota_error_t error) {
      otaInProgress = false;
    });

  ArduinoOTA.begin();
}

void otaLoop()
{
  ArduinoOTA.handle();
}
