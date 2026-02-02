#pragma once

#include <Arduino.h>
#include <time.h>

/* =========================
   Control / Process Modes
   ========================= */

enum ControlMode : uint8_t {
  CONTROL_UNMANAGED = 0,
  CONTROL_MANAGED   = 1
};

enum ProcessType : uint8_t {
  PROCESS_NONE = 0,
  PROCESS_EGG_HOLDING,
  PROCESS_INCUBATION
};

/* =========================
   Egg Profile IDs
   ========================= */

enum EggProfileId : uint8_t {
  PROFILE_CHICKEN = 0,
  PROFILE_COCKATIEL,
  PROFILE_CORMORANT,
  PROFILE_CRANE,
  PROFILE_DUCK,
  PROFILE_DUCK_MUSCOVY,
  PROFILE_EAGLE,
  PROFILE_EMU,
  PROFILE_FALCON,
  PROFILE_FLAMINGO,
  PROFILE_GOOSE,
  PROFILE_GROUSE,
  PROFILE_GUINEA_FOWL,
  PROFILE_HAWK,
  PROFILE_HERON,
  PROFILE_HUMMINGBIRD,
  PROFILE_LARGE_PARROTS,
  PROFILE_LOVEBIRD,
  PROFILE_OSTRICH,
  PROFILE_OWL,
  PROFILE_PARAKEET,
  PROFILE_PARROTS,
  PROFILE_PARTRIDGE,
  PROFILE_PEACOCK,
  PROFILE_PELICAN,
  PROFILE_PENGUIN,
  PROFILE_PHEASANT,
  PROFILE_PIGEON,
  PROFILE_QUAIL,
  PROFILE_RAIL,
  PROFILE_RHEA,
  PROFILE_SEABIRDS,
  PROFILE_SONGBIRDS,
  PROFILE_STORK,
  PROFILE_SWAN,
  PROFILE_TOUCAN,
  PROFILE_TURKEY,
  PROFILE_VULTURE,
  PROFILE_CUSTOM
};

/* =========================
   Incubation Phase (shared by profiles and ProcessState)
   ========================= */

struct IncubationPhase {
  uint16_t startDay;          /* inclusive */
  uint16_t endDay;            /* inclusive */
  float tempMinF;
  float tempMaxF;
  float humMin;
  float humMax;
  bool turningEnabled;
  uint16_t turnIntervalHours; /* 0 = disabled */
};

/* =========================
   Static Egg Profile Data
   ========================= */
// Note: EggProfileData is defined in profiles_module.h

/* =========================
   Runtime Process State
   ========================= */

struct ProcessState {
  bool active;

  ControlMode controlMode;
  ProcessType processType;

  uint8_t profileId;

  time_t startEpoch;
  uint16_t startDay;
  uint16_t currentDay;

  uint8_t activePhaseIndex;

  time_t lastTurnEpoch;

  /* Custom profile: phase-based (same model as presets). */
  IncubationPhase customPhases[4];
  uint8_t customPhaseCount;
};

extern ProcessState process;

/* =========================
   Global Targets (active)
   ========================= */

extern float targetMinF;
extern float targetMaxF;
extern float targetHMin;
extern float targetHMax;

extern EggProfileId currentProfile;

/* =========================
   System Flags
   ========================= */

extern volatile bool otaInProgress;

/* =========================
   Info section (UI: WiFi, AP SSID, WebSocket, Mode, IPs, MAC)
   Single source of truth; wifi/ws modules update, frontend reads via WebSocket.
   ========================= */

void appstate_setApSsid(const char *ssid);
void appstate_setStaSsid(const char *ssid);
void appstate_setWifiInfo(bool connected, const char *staIp, const char *apIp, const char *mac);
/** Set WiFi STA RSSI in dBm when connected; use -128 when not connected (no signal). */
void appstate_setWifiRssi(int8_t rssi);
void appstate_setWsConnected(bool connected);
void appstate_setLamp(bool on);
void appstate_setHumidifier(bool on);
void appstate_setSystemEnabled(bool on);
/** Manual mode: egg turning on/off and interval (hours). Used when process is not active. */
void appstate_setManualTurning(bool enabled, uint16_t intervalHours);
bool appstate_getManualTurningEnabled();
uint16_t appstate_getManualTurnIntervalHours();

const char *appstate_getApSsid();
const char *appstate_getStaSsid();
bool appstate_getWifiConnected();
/** RSSI in dBm when connected; -128 when not connected. */
int appstate_getWifiRssi();
const char *appstate_getWifiIp();
const char *appstate_getApIp();
const char *appstate_getMac();
bool appstate_getWsConnected();
bool appstate_getLamp();
bool appstate_getHumidifier();
/* System enabled flag: when false, sensors still read but no lamp/humidifier output. Exposed for inline getter. */
extern bool info_system_enabled;
inline bool appstate_getSystemEnabled() { return info_system_enabled; }
/* Mode derived from process.controlMode. */
const char *appstate_getDisplayMode();

/* =========================
   Functions
   ========================= */

void saveProcessState();
void resetProcessState();
void appstate_setManualTargets(float tmin, float tmax, float hmin, float hmax);
bool isCustomProfileActive();
bool isProcessRunning();

/* Module lifecycle */
void appstate_setup();
void appstate_loop();
