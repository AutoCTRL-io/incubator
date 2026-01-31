#include <Arduino.h>
#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h>

#include "webserver_module.h"

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
        const char *path = nullptr;
        if (upload.name == "index") path = PATH_INDEX;
        else if (upload.name == "wifi") path = PATH_WIFI;
        else if (upload.name == "style") path = PATH_STYLE;
        else if (upload.name == "app") path = PATH_APP;
        if (path)
          s_uploadFile = LittleFS.open(path, "w");
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
