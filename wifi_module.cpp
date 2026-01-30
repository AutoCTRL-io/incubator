#include "wifi_module.h"
#include <Preferences.h>

static bool apRunning = false;
static const char* WIFI_NVS_NS = "wifi";
static const char* K_STA_SSID = "sta_ssid";
static const char* K_STA_PASS = "sta_pass";

/* Print STA IP to serial every 1 second so user can see when/if it gets an address. */
static const unsigned long WIFI_LOOP_INTERVAL_MS = 1000;
static unsigned long wifiLastPrintMs = 0;

void wifiSaveCredentials(const char* ssid, const char* password)
{
  Preferences prefs;
  prefs.begin(WIFI_NVS_NS, false);
  prefs.putString(K_STA_SSID, ssid ? ssid : "");
  prefs.putString(K_STA_PASS, password ? password : "");
  prefs.end();
}

bool wifi_setup(const char* apSsid, const char* apPassword)
{
  Serial.println("WiFi: Initializing...");
  Serial.flush();

  Preferences prefs;
  prefs.begin(WIFI_NVS_NS, true);
  String staSsid = prefs.getString(K_STA_SSID, "");
  String staPass = prefs.getString(K_STA_PASS, "");
  prefs.end();

  if (staSsid.length() > 0) {
    WiFi.mode(WIFI_AP_STA);
    Serial.println("WiFi: Mode set to AP+STA (using saved credentials)");
    Serial.flush();
    delay(100);

    bool apOk = WiFi.softAP(apSsid, apPassword);
    if (!apOk) {
      Serial.println("WiFi: ERROR - Failed to start AP!");
      Serial.flush();
      apRunning = false;
      return false;
    }
    apRunning = true;
    delay(500);

    Serial.print("WiFi: Connecting to ");
    Serial.print(staSsid);
    Serial.println("...");
    Serial.flush();
    WiFi.begin(staSsid.c_str(), staPass.c_str());

    const int timeoutMs = 10000;
    const int stepMs = 500;
    int waited = 0;
    while (WiFi.status() != WL_CONNECTED && waited < timeoutMs) {
      delay(stepMs);
      waited += stepMs;
      if (waited % 2000 == 0) {
        Serial.print("  ... waiting ");
        Serial.print(waited / 1000);
        Serial.println("s");
        Serial.flush();
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi: STA connected, IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi: STA connection timeout (AP only)");
    }
    Serial.flush();
    return true;
  }

  WiFi.mode(WIFI_AP);
  Serial.println("WiFi: Mode set to AP only (no saved credentials)");
  Serial.flush();
  delay(100);

  bool started = WiFi.softAP(apSsid, apPassword);
  if (!started) {
    Serial.println("WiFi: ERROR - Failed to start AP!");
    Serial.flush();
    apRunning = false;
    return false;
  }

  delay(500);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("WiFi: AP started, IP: ");
  Serial.println(ip);
  Serial.flush();
  apRunning = true;
  return true;
}

void wifi_loop()
{
  unsigned long now = millis();
  if (now - wifiLastPrintMs >= WIFI_LOOP_INTERVAL_MS) {
    wifiLastPrintMs = now;
    IPAddress ap = wifiGetAPIP();
    IPAddress sta = wifiGetSTAIP();
    Serial.print("AP IP: ");
    Serial.print(ap);
    Serial.print("  STA IP: ");
    Serial.println(sta);
    Serial.flush();
  }
}

IPAddress wifiGetAPIP()
{
  if (!apRunning) {
    return IPAddress(0, 0, 0, 0);
  }
  return WiFi.softAPIP();
}

IPAddress wifiGetSTAIP()
{
  if (WiFi.status() != WL_CONNECTED) {
    return IPAddress(0, 0, 0, 0);
  }
  return WiFi.localIP();
}

bool wifiIsAPRunning()
{
  return apRunning;
}

bool wifiIsSTAConnected()
{
  return WiFi.status() == WL_CONNECTED;
}
