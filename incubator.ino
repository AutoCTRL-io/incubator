#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "app_state.h"
#include "sensor_dht.h"
#include "profiles.h"
#include "core_controller.h"
#include "motor_stepper.h"
#include "ota_manager.h"
#include "web_server.h"
#include "ws_server.h"

/* ===== Minimal Debug Setup ===== */
void setup()
{
  Serial.begin(115200);
  delay(2000); // Extra delay for Serial to stabilize
  Serial.println("\n\n=== MINIMAL BOOT TEST ===");
  Serial.flush();
  
  Serial.println("Step 1: Basic Serial OK");
  Serial.flush();
  delay(100);
  
  Serial.println("Step 2: Testing WiFi initialization...");
  Serial.flush();
  WiFi.mode(WIFI_AP);
  Serial.println("  - WiFi mode set");
  Serial.flush();
  delay(100);
  
  bool apStarted = WiFi.softAP("Incubator", "12345678");
  if (apStarted) {
    Serial.print("  - AP started, IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("  - AP FAILED to start");
  }
  Serial.flush();
  delay(500);
  
  Serial.println("Step 3: Testing sensorInit...");
  Serial.flush();
  sensorInit(4);
  Serial.println("  - sensorInit OK");
  Serial.flush();
  delay(100);
  
  Serial.println("Step 4: Testing stepperInit...");
  Serial.flush();
  StepperConfig cfg = {
    .pinStep = 12,
    .pinDir = 13,
    .pinEnable = 14,
    .stepsPerTurn = 800,
    .invertDir = false
  };
  stepperInit(cfg);
  Serial.println("  - stepperInit OK");
  Serial.flush();
  delay(100);
  
  Serial.println("Step 5: Testing loadProcessState...");
  Serial.flush();
  loadProcessState();
  Serial.println("  - loadProcessState OK");
  Serial.flush();
  delay(100);
  
  Serial.println("Step 6: Testing coreInit...");
  Serial.flush();
  coreInit();
  Serial.println("  - coreInit OK");
  Serial.flush();
  delay(100);
  
  Serial.println("Step 7: Testing webServerInit...");
  Serial.flush();
  WebServer server(80);
  webServerInit(server);
  Serial.println("  - webServerInit OK");
  Serial.flush();
  delay(100);
  
  Serial.println("Step 8: Testing wsServerInit...");
  Serial.flush();
  WebSocketsServer ws(81);
  wsServerInit(ws);
  Serial.println("  - wsServerInit OK");
  Serial.flush();
  delay(100);
  
  Serial.println("Step 9: Testing otaInit...");
  Serial.flush();
  otaInit();
  Serial.println("  - otaInit OK");
  Serial.flush();
  delay(100);
  
  Serial.println("\n=== ALL INITIALIZATION COMPLETE ===");
  Serial.println("Device should be running now.");
  Serial.println("If you see this message, the crash is in loop()");
  Serial.flush();
}

void loop()
{
  Serial.println("Loop running...");
  delay(5000);
}
