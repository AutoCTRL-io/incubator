#include "wifi_module.h"
#include "appstate_module.h"
#include <Preferences.h>

static bool apRunning = false;
/* Match original_code.ino: credentials saved in namespace "incubator" with keys "ssid" and "pass". */
static const char* WIFI_NVS_NS = "incubator";
static const char* K_SSID = "ssid";
static const char* K_PASS = "pass";

/* Print AP/STA IP to serial every 10s so logs stay readable; DHT prints every 2s. */
static const unsigned long WIFI_LOOP_INTERVAL_MS = 10000;
static unsigned long wifiLastPrintMs = 0;

void wifiSaveCredentials(const char* ssid, const char* password)
{
  Preferences prefs;
  prefs.begin(WIFI_NVS_NS, false);
  prefs.putString(K_SSID, ssid ? ssid : "");
  prefs.putString(K_PASS, password ? password : "");
  prefs.end();
}

bool wifi_setup(const char* apSsid, const char* apPassword, const char* staSsid, const char* staPass)
{
  Serial.println("WiFi: Initializing...");
  Serial.flush();

  /* Credentials come from loader; no NVS read here. */
  bool haveStaCreds = (staSsid != nullptr && strlen(staSsid) > 0);
  const char* pass = (staPass != nullptr) ? staPass : "";

  if (haveStaCreds) {
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setHostname("Incubator");
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
    WiFi.begin(staSsid, pass);

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
      /* Wait for DHCP to assign STA IP (can be a moment after WL_CONNECTED). */
      int dhcpWaitMs = 0;
      const int dhcpTimeoutMs = 5000;
      while (WiFi.localIP() == IPAddress(0, 0, 0, 0) && dhcpWaitMs < dhcpTimeoutMs) {
        delay(200);
        dhcpWaitMs += 200;
      }
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
  /* Keep appstate info section current so WebSocket can send it on connect / when changed. */
  {
    bool connected = (WiFi.status() == WL_CONNECTED);
    String staStr = WiFi.localIP().toString();
    String apStr = wifiGetAPIP().toString();
    String macStr = WiFi.macAddress();
    appstate_setWifiInfo(connected, staStr.c_str(), apStr.c_str(), macStr.c_str());
  }

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
