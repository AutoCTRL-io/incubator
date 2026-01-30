#include <Arduino.h>
#include <WebServer.h>
#include "wifi_module.h"
#include "webserver_module.h"

WebServer server(80);

/* ===== Serial + WiFi + Web Server (minimal: wifi + webserver modules only) ===== */
void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n=== Incubator - WiFi + Web ===");
  Serial.flush();

  if (wifi_setup("Incubator", "12345678")) {
    Serial.println("WiFi: SUCCESS");
    Serial.print("AP IP: ");
    Serial.println(wifiGetAPIP());
    Serial.print("WiFi IP: ");
    Serial.println(wifiGetSTAIP());
  } else {
    Serial.println("WiFi: FAILED");
  }
  Serial.flush();

  webserver_setup(server);
  Serial.print("Web server: started (http://");
  Serial.print(wifiGetAPIP());
  Serial.println("/)");
  Serial.print("Web server: started (http://");
  Serial.print(wifiGetSTAIP());
  Serial.println("/)");
  Serial.println("(WiFi IP will be printed every 1s in loop)");
  Serial.flush();
}

void loop()
{
  wifi_loop();
  webserver_loop(server);
}
