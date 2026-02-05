#include "ota_module.h"
#include <ArduinoOTA.h>
#include <Preferences.h>

volatile bool otaInProgress = false;

/* OTA password is stored in NVS namespace "incubator", key "ota_pass". Applied at setup; set via Settings page. */
static const char* NVS_NAMESPACE = "incubator";
static const char* NVS_KEY_OTA_PASS = "ota_pass";

void ota_setup()
{
  ArduinoOTA.setHostname("incubator");

  /* Load OTA password from NVS if user set one via Settings page. */
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    String pass = prefs.getString(NVS_KEY_OTA_PASS, "");
    prefs.end();
    if (pass.length() > 0) {
      ArduinoOTA.setPassword(pass.c_str());
    }
  }

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
