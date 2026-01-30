#pragma once

#include <Arduino.h>
#include <WiFi.h>

/* Module lifecycle */
bool wifi_setup(const char* apSsid, const char* apPassword);
void wifi_loop();

IPAddress wifiGetAPIP();
IPAddress wifiGetSTAIP();
bool wifiIsAPRunning();
bool wifiIsSTAConnected();

void wifiSaveCredentials(const char* ssid, const char* password);
