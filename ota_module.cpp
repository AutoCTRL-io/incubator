#include "ota_module.h"
#include <ArduinoOTA.h>

volatile bool otaInProgress = false;

void ota_setup()
{
  ArduinoOTA.setHostname("incubator");
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

void ota_loop()
{
  ArduinoOTA.handle();
}
