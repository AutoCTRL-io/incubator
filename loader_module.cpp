#include "loader_module.h"
#include "appstate_module.h"
#include <Preferences.h>

static const char* NVS_NAMESPACE = "incubator";

/* WiFi credentials loaded from NVS; wifi_setup uses these via loader_getStaSsid/Pass. */
static String loadedStaSsid;
static String loadedStaPass;

void loader_setup()
{
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, true); // read-only

  /* ---- WiFi (original_code.ino keys) ---- */
  loadedStaSsid = prefs.getString("ssid", "");
  loadedStaPass = prefs.getString("pass", "");
  /* keepap not used by wifi_module yet; can be read here if needed later */

  /* ---- Target overrides (original_code.ino keys) ---- */
  if (prefs.isKey("tmin") && prefs.isKey("tmax")) {
    targetMinF = prefs.getFloat("tmin", targetMinF);
    targetMaxF = prefs.getFloat("tmax", targetMaxF);
  }
  if (prefs.isKey("hmin") && prefs.isKey("hmax")) {
    targetHMin = prefs.getFloat("hmin", targetHMin);
    targetHMax = prefs.getFloat("hmax", targetHMax);
  }

  /* ---- Profile (original_code.ino key) ---- */
  if (prefs.isKey("profile")) {
    uint8_t p = prefs.getUChar("profile", PROFILE_CHICKEN);
    if (p <= PROFILE_CUSTOM) {
      currentProfile = (EggProfileId)p;
      process.profileId = p;
    }
  }

  /* ---- Full process state (appstate saveProcessState keys) ---- */
  if (prefs.getBool("valid", false)) {
    process.active = prefs.getBool("active", false);
    process.controlMode = (ControlMode)prefs.getUChar("controlMode", CONTROL_UNMANAGED);
    process.processType = (ProcessType)prefs.getUChar("processType", PROCESS_NONE);

    uint8_t loadedProfileId = prefs.getUChar("profileId", PROFILE_CHICKEN);
    if (loadedProfileId <= PROFILE_CUSTOM) {
      process.profileId = loadedProfileId;
      currentProfile = (EggProfileId)loadedProfileId;
    }

    process.startEpoch = (time_t)prefs.getULong64("startEpoch", 0);
    process.startDay = prefs.getUShort("startDay", 1);
    process.currentDay = prefs.getUShort("currentDay", 0);
    process.lastTurnEpoch = (time_t)prefs.getULong64("lastTurnEpoch", 0);

    process.customMinF = prefs.getFloat("customMinF", NAN);
    process.customMaxF = prefs.getFloat("customMaxF", NAN);
    process.customHumMin = prefs.getFloat("customHumMin", NAN);
    process.customHumMax = prefs.getFloat("customHumMax", NAN);
    process.customTotalDays = prefs.getUShort("customTotalDays", 0);
    process.customTurnsPerDay = prefs.getUChar("customTurnsPerDay", 0);
  }

  prefs.end();

  Serial.print("Loader: WiFi SSID ");
  Serial.println(loadedStaSsid.length() > 0 ? loadedStaSsid.c_str() : "(none)");
  Serial.flush();
}

void loader_loop()
{
  /* No periodic work. */
}

const char* loader_getStaSsid()
{
  return loadedStaSsid.length() > 0 ? loadedStaSsid.c_str() : nullptr;
}

const char* loader_getStaPass()
{
  /* When ssid is set, pass may be "" (open network); always return valid C string. */
  return loadedStaPass.c_str();
}
