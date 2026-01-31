#pragma once

#include <Arduino.h>
#include <WiFi.h>

/* Module lifecycle. STA credentials come from loader (pass NULL if none). */
bool wifi_setup(const char* apSsid, const char* apPassword, const char* staSsid, const char* staPass);
void wifi_loop();

IPAddress wifiGetAPIP();
IPAddress wifiGetSTAIP();
bool wifiIsAPRunning();
bool wifiIsSTAConnected();

void wifiSaveCredentials(const char* ssid, const char* password);
