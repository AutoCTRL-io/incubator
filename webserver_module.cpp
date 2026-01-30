#include <Arduino.h>
#include <WebServer.h>

#include "webserver_module.h"
#include "webassets_module.h"

void webserver_setup(WebServer &server)
{
  /* ===== Pages ===== */
  server.on("/", HTTP_GET, [&]() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/wifi", HTTP_GET, [&]() {
    server.send_P(200, "text/html", WIFI_HTML);
  });

  /* ===== Static Assets ===== */
  server.on("/style.css", HTTP_GET, [&]() {
    server.send_P(200, "text/css", STYLES);
  });

  server.on("/app.js", HTTP_GET, [&]() {
    server.send_P(200, "application/javascript", JAVASCRIPTS);
  });

  server.begin();
}

void webserver_loop(WebServer &server)
{
  server.handleClient();
}
