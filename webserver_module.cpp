#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#include "webserver_module.h"
#include "wifi_module.h"

/* LittleFS paths for uploaded web assets. */
static const char PATH_INDEX[] = "/index.html";
static const char PATH_WIFI[] = "/wifi.html";
static const char PATH_STYLE[] = "/style.css";
static const char PATH_APP[] = "/app.js";

/* Fallback HTML when an asset is not uploaded yet. */
static void sendFallback(WebServer &server, const char *assetName) {
  String body = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"/>"
    "<title>Upload</title></head><body><p>Upload ";
  body += assetName;
  body += " at <a href=\"/upload\">/upload</a>.</p><p><a href=\"/\">Back to home</a></p></body></html>";
  server.send(200, "text/html", body);
}

/* Form for uploading all web assets (GET /upload). */
static const char UPLOAD_FORM_HTML[] PROGMEM =
  "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"/>"
  "<title>Upload</title></head><body>"
  "<h1>Upload web assets</h1>"
  "<p>Upload each file to serve from the device. If a file is not uploaded, that route shows a message with a link here.</p>"
  "<form action=\"/upload\" method=\"POST\" enctype=\"multipart/form-data\">"
  "<p><label>index.html (main page at /): <input type=\"file\" name=\"index\"/></label></p>"
  "<p><label>wifi.html (settings at /wifi): <input type=\"file\" name=\"wifi\"/></label></p>"
  "<p><label>style.css: <input type=\"file\" name=\"style\"/></label></p>"
  "<p><label>app.js: <input type=\"file\" name=\"app\"/></label></p>"
  "<button type=\"submit\">Upload selected</button></form>"
  "<p><a href=\"/\">Back to home</a></p></body></html>";

/* File handle kept open across upload chunk callbacks. */
static File s_uploadFile;

void webserver_setup(WebServer &server)
{
  LittleFS.begin(true);  /* Mount; format if mount fails. */

  /* ===== Reachability test ===== */
  server.on("/ping", HTTP_GET, [&]() {
    server.send(200, "text/plain", "OK");
  });

  /* ===== Settings API: WiFi and OTA password (NVS namespace "incubator") ===== */
  static const char* NVS_NS = "incubator";
  static const char* NVS_KEY_OTA_PASS = "ota_pass";

  /* GET /api/wifi -> { ssid, keep_ap, ota_password_set }. Does not return plaintext OTA password. */
  server.on("/api/wifi", HTTP_GET, [&]() {
    Preferences prefs;
    prefs.begin(NVS_NS, true);
    String ssid = prefs.getString("ssid", "");
    bool keep_ap = prefs.getBool("keep_ap", true);
    String otaPass = prefs.getString(NVS_KEY_OTA_PASS, "");
    prefs.end();
    StaticJsonDocument<256> doc;
    doc["ssid"] = ssid;
    doc["keep_ap"] = keep_ap;
    doc["ota_password_set"] = (otaPass.length() > 0);
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  /* POST /api/wifi -> body { ssid, pass?, keep_ap?, ota_password? }. Saves WiFi creds and optional OTA password to NVS. */
  server.on("/api/wifi", HTTP_POST, [&]() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"Missing body\"}");
      return;
    }
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
      return;
    }
    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["pass"] | "";
    bool keep_ap = doc["keep_ap"] | true;
    const char* ota_password = doc["ota_password"] | "";

    wifiSaveCredentials(ssid, pass);

    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putBool("keep_ap", keep_ap);
    /* Store OTA password (empty string clears it). Max 63 chars for NVS string. */
    String op(ota_password);
    if (op.length() > 63) op = op.substring(0, 63);
    prefs.putString(NVS_KEY_OTA_PASS, op);
    prefs.end();

    server.send(200, "application/json", "{\"ok\":true}");
  });

  /* GET /api/wifi/scan -> { networks: [ { ssid, rssi, enc }, ... ] } */
  server.on("/api/wifi/scan", HTTP_GET, [&]() {
    int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.createNestedArray("networks");
    for (int i = 0; i < n; i++) {
      JsonObject obj = arr.add<JsonObject>();
      obj["ssid"] = WiFi.SSID(i);
      obj["rssi"] = WiFi.RSSI(i);
      obj["enc"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    WiFi.scanDelete();
    String str;
    serializeJson(doc, str);
    server.send(200, "application/json", str);
  });

  /* POST /api/reset -> reboot device */
  server.on("/api/reset", HTTP_POST, [&]() {
    server.send(200, "application/json", "{\"ok\":true}");
    delay(200);
    ESP.restart();
  });

  /* ===== Main page: from LittleFS if uploaded, else fallback ===== */
  server.on("/", HTTP_GET, [&]() {
    File f = LittleFS.open(PATH_INDEX, "r");
    if (f && f.size() > 0) {
      server.streamFile(f, "text/html");
      f.close();
    } else {
      sendFallback(server, "index.html");
    }
  });

  /* ===== Upload form (GET) ===== */
  server.on("/upload", HTTP_GET, [&]() {
    server.send_P(200, "text/html", UPLOAD_FORM_HTML);
  });

  /* ===== Upload handler (POST): save each file by field name (index, wifi, style, app) ===== */
  /* Only open a file when the user actually selected one (non-empty filename); otherwise
   * we would open with "w" and truncate existing content for empty form fields. */
  server.on("/upload", HTTP_POST,
    [&]() {
      server.send(200, "text/html",
        "<p>Upload complete. <a href=\"/\">Back to home</a></p>");
    },
    [&]() {
      HTTPUpload &upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        if (s_uploadFile)
          s_uploadFile.close();
        s_uploadFile = File();
        /* Only open if user selected a file for this field (filename non-empty). */
        if (upload.filename && upload.filename[0] != '\0') {
          const char *path = nullptr;
          if (upload.name == "index") path = PATH_INDEX;
          else if (upload.name == "wifi") path = PATH_WIFI;
          else if (upload.name == "style") path = PATH_STYLE;
          else if (upload.name == "app") path = PATH_APP;
          if (path)
            s_uploadFile = LittleFS.open(path, "w");
        }
      } else if (upload.status == UPLOAD_FILE_WRITE && s_uploadFile) {
        s_uploadFile.write(upload.buf, upload.currentSize);
      } else if (upload.status == UPLOAD_FILE_END) {
        if (s_uploadFile) {
          s_uploadFile.close();
          s_uploadFile = File();
        }
      }
    });

  /* ===== /wifi: from LittleFS if uploaded, else fallback ===== */
  server.on("/wifi", HTTP_GET, [&]() {
    File f = LittleFS.open(PATH_WIFI, "r");
    if (f && f.size() > 0) {
      server.streamFile(f, "text/html");
      f.close();
    } else {
      sendFallback(server, "wifi.html");
    }
  });

  /* ===== /style.css: from LittleFS if uploaded, else fallback ===== */
  server.on("/style.css", HTTP_GET, [&]() {
    File f = LittleFS.open(PATH_STYLE, "r");
    if (f && f.size() > 0) {
      server.streamFile(f, "text/css");
      f.close();
    } else {
      sendFallback(server, "style.css");
    }
  });

  /* ===== /app.js: from LittleFS if uploaded, else fallback ===== */
  server.on("/app.js", HTTP_GET, [&]() {
    File f = LittleFS.open(PATH_APP, "r");
    if (f && f.size() > 0) {
      server.streamFile(f, "application/javascript");
      f.close();
    } else {
      sendFallback(server, "app.js");
    }
  });

  /* ===== 404 ===== */
  server.onNotFound([&]() {
    server.send(404, "text/plain", "Not Found");
  });

  delay(200);
  server.begin();
}

void webserver_loop(WebServer &server)
{
  server.handleClient();
}
