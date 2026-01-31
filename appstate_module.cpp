#include "appstate_module.h"
#include <Preferences.h>
#include <string.h>

/* =========================
   Global Runtime State
   ========================= */

ProcessState process = {
  .active = false,

  .controlMode = CONTROL_UNMANAGED,
  .processType = PROCESS_NONE,

  .profileId = PROFILE_CHICKEN,

  .startEpoch = 0,
  .startDay = 1,
  .currentDay = 0,

  .lastTurnEpoch = 0,

  .customMinF = NAN,
  .customMaxF = NAN,
  .customHumMin = NAN,
  .customHumMax = NAN,
  .customTotalDays = 0,
  .customTurnsPerDay = 0
};

/* =========================
   Global Targets (active)
   ========================= */

float targetMinF = 98.0f;
float targetMaxF = 100.5f;
float targetHMin = 40.0f;
float targetHMax = 60.0f;

EggProfileId currentProfile = PROFILE_CHICKEN;

/* =========================
   System Flags
   ========================= */
// Note: otaInProgress is defined in ota_module.cpp

/* =========================
   Info section (UI)
   ========================= */

#define INFO_AP_SSID_LEN 32
#define INFO_IP_LEN 16
#define INFO_MAC_LEN 18

static char info_ap_ssid[INFO_AP_SSID_LEN] = "";
static char info_sta_ssid[INFO_AP_SSID_LEN] = "";
static bool info_wifi_connected = false;
static char info_wifi_ip[INFO_IP_LEN] = "";
static char info_ap_ip[INFO_IP_LEN] = "";
static char info_mac[INFO_MAC_LEN] = "";
static bool info_ws_connected = false;
static bool info_lamp = false;

void appstate_setApSsid(const char *ssid)
{
  if (!ssid) return;
  strncpy(info_ap_ssid, ssid, INFO_AP_SSID_LEN - 1);
  info_ap_ssid[INFO_AP_SSID_LEN - 1] = '\0';
}

void appstate_setStaSsid(const char *ssid)
{
  if (!ssid) return;
  strncpy(info_sta_ssid, ssid, INFO_AP_SSID_LEN - 1);
  info_sta_ssid[INFO_AP_SSID_LEN - 1] = '\0';
}

void appstate_setWifiInfo(bool connected, const char *staIp, const char *apIp, const char *mac)
{
  info_wifi_connected = connected;
  if (staIp) {
    strncpy(info_wifi_ip, staIp, INFO_IP_LEN - 1);
    info_wifi_ip[INFO_IP_LEN - 1] = '\0';
  } else {
    info_wifi_ip[0] = '\0';
  }
  if (apIp) {
    strncpy(info_ap_ip, apIp, INFO_IP_LEN - 1);
    info_ap_ip[INFO_IP_LEN - 1] = '\0';
  } else {
    info_ap_ip[0] = '\0';
  }
  if (mac) {
    strncpy(info_mac, mac, INFO_MAC_LEN - 1);
    info_mac[INFO_MAC_LEN - 1] = '\0';
  } else {
    info_mac[0] = '\0';
  }
}

void appstate_setWsConnected(bool connected)
{
  info_ws_connected = connected;
}

void appstate_setLamp(bool on)
{
  info_lamp = on;
}

const char *appstate_getApSsid()
{
  return info_ap_ssid;
}

const char *appstate_getStaSsid()
{
  return info_sta_ssid;
}

bool appstate_getWifiConnected()
{
  return info_wifi_connected;
}

const char *appstate_getWifiIp()
{
  return info_wifi_ip;
}

const char *appstate_getApIp()
{
  return info_ap_ip;
}

const char *appstate_getMac()
{
  return info_mac;
}

bool appstate_getWsConnected()
{
  return info_ws_connected;
}

bool appstate_getLamp()
{
  return info_lamp;
}

const char *appstate_getDisplayMode()
{
  return process.controlMode == CONTROL_MANAGED ? "Managed" : "Unmanaged";
}

/* =========================
   NVS Persistence (save only; load is done by loader_module)
   ========================= */

static Preferences prefs;
static const char* NVS_NAMESPACE = "incubator";

void saveProcessState()
{
  prefs.begin(NVS_NAMESPACE, false); // read-write

  prefs.putBool("valid", true);

  prefs.putBool("active", process.active);
  prefs.putUChar("controlMode", (uint8_t)process.controlMode);
  prefs.putUChar("processType", (uint8_t)process.processType);
  prefs.putUChar("profileId", process.profileId);

  prefs.putULong64("startEpoch", (uint64_t)process.startEpoch);
  prefs.putUShort("startDay", process.startDay);
  prefs.putUShort("currentDay", process.currentDay);

  prefs.putULong64("lastTurnEpoch", (uint64_t)process.lastTurnEpoch);

  prefs.putFloat("customMinF", process.customMinF);
  prefs.putFloat("customMaxF", process.customMaxF);
  prefs.putFloat("customHumMin", process.customHumMin);
  prefs.putFloat("customHumMax", process.customHumMax);
  prefs.putUShort("customTotalDays", process.customTotalDays);
  prefs.putUChar("customTurnsPerDay", process.customTurnsPerDay);

  prefs.end();
}

/* =========================
   Helpers
   ========================= */

void resetProcessState()
{
  process.active = false;
  process.controlMode = CONTROL_UNMANAGED;
  process.processType = PROCESS_NONE;
  process.profileId = PROFILE_CHICKEN;

  process.startEpoch = 0;
  process.startDay = 1;
  process.currentDay = 0;
  process.lastTurnEpoch = 0;

  process.customMinF = NAN;
  process.customMaxF = NAN;
  process.customHumMin = NAN;
  process.customHumMax = NAN;
  process.customTotalDays = 0;
  process.customTurnsPerDay = 0;

  saveProcessState();
}

bool isCustomProfileActive()
{
  return process.profileId == PROFILE_CUSTOM;
}

bool isProcessRunning()
{
  return process.active && process.processType != PROCESS_NONE;
}

/* =========================
   Module lifecycle
   ========================= */

/* Establish default values only. Loader runs after this and overwrites from NVS if data exists. */
void appstate_setup()
{
  /* Defaults already set by static initialization above; no NVS read here. */
}

void appstate_loop()
{
  /* No periodic work. */
}
