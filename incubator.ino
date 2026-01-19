/*
  ESP32-S3 Incubator Controller
  - DHT22 temp/RH + computed absolute humidity
  - Relay heat control (HIGH = ON)
  - Web UI (minimal) + WiFi config page
  - WebSockets real-time status (event-driven, 1s)
  - Preferences (NVS) for target range + WiFi creds + "Keep AP when connected"
  - STA (DHCP) + AP fallback (SSID: Incubator, PASS: hookerface)
  - ArduinoOTA (password: hookerface)

  Libraries (Arduino Library Manager):
    - DHT sensor library (Adafruit)
    - Adafruit Unified Sensor
    - WebSockets (by Markus Sattler)  // "WebSockets"
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>
#include <DHT.h>
#include <math.h>
#include <time.h>
#include <esp_wifi.h>
#include <esp_system.h>


// ===== Pins =====
#define DHTPIN 4
#define DHTTYPE DHT22
#define RELAY_PIN 5  // HIGH = heat ON

// ===== Timing =====
static const uint32_t SENSOR_INTERVAL_MS = 2000;  // 1s
static const uint16_t WS_PORT = 81;

// ===== AP Defaults =====
static const char* AP_SSID = "Incubator";
static const char* AP_PASS = "hookerface";

// ===== OTA =====
static const char* OTA_PASS = "hookerface";

// ===== Acceptable limits (as requested) =====
static const float TEMP_MIN_ALLOWED_F = -60.0f;
static const float TEMP_MAX_ALLOWED_F = 200.0f;

// ===== Ideal defaults for hatching chicken eggs =====
static const float DEFAULT_TMIN_F = 98.0f;
static const float DEFAULT_TMAX_F = 100.5f;

static const uint8_t MAX_TEMP_PEAKS = 5;
static const uint32_t TEMP_PEAK_WINDOW_SEC = 6 * 60 * 60;

struct TempPeak {
  float tempF;
  time_t ts;
};

TempPeak tempPeaks[MAX_TEMP_PEAKS];
uint8_t tempPeakCount = 0;

// ===== Globals =====
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
WebSocketsServer ws(WS_PORT);
Preferences prefs;

String staSsid, staPass;
bool keepApWhenConnected = true;

float targetMinF = DEFAULT_TMIN_F;
float targetMaxF = DEFAULT_TMAX_F;

bool relayOn = false;
bool lastRelayOn = false;

float lastTempC = NAN;
float lastTempF = NAN;
float lastRH = NAN;
float lastAbsH = NAN;
float lastDewC = NAN;
float lastDewF = NAN;
float lastHeatC = NAN;
float lastHeatF = NAN;


uint32_t lastSensorMs = 0;

enum EggProfile : uint8_t {
  PROFILE_CHICKEN = 0,
  PROFILE_QUAIL,
  PROFILE_DUCK,
  PROFILE_TURKEY,
  PROFILE_GOOSE,
  PROFILE_CUSTOM
};

struct ProfilePreset {
  const char* name;
  float tmin;
  float tmax;
};

static const ProfilePreset PROFILE_PRESETS[] = {
  { "Chicken", 98.0f, 100.5f },
  { "Quail", 99.5f, 101.0f },
  { "Duck", 99.5f, 100.0f },
  { "Turkey", 99.0f, 100.0f },
  { "Goose", 99.0f, 100.0f },
  { "Custom", 98.0f, 100.5f }  // placeholder
};

EggProfile currentProfile = PROFILE_CHICKEN;

static inline float cToF(float c) {
  return (c * 9.0f / 5.0f) + 32.0f;
}

// Absolute humidity (g/m^3) from tempC and RH%
// AH = 216.7 * ( (RH/100) * 6.112 * exp(17.67*T/(T+243.5)) ) / (T+273.15)
float absoluteHumidity_gm3(float tempC, float rh) {
  float T = tempC;
  float RH = rh;
  float es = 6.112f * expf((17.67f * T) / (T + 243.5f));  // hPa
  float e = (RH / 100.0f) * es;                           // hPa
  float ah = 216.7f * (e / (T + 273.15f));                // g/m^3
  return ah;
}

String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '\"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

String wifiStatusString() {
  if (WiFi.status() == WL_CONNECTED) return "CONNECTED";
  return "DISCONNECTED";
}

void recordTempPeak(float tempF) {
  time_t now;
  time(&now);
  if (now < 100000) return;

  uint8_t w = 0;
  for (uint8_t i = 0; i < tempPeakCount; i++) {
    if ((now - tempPeaks[i].ts) <= TEMP_PEAK_WINDOW_SEC) {
      tempPeaks[w++] = tempPeaks[i];
    }
  }
  tempPeakCount = w;

  if (tempPeakCount < MAX_TEMP_PEAKS) {
    tempPeaks[tempPeakCount++] = { tempF, now };
    return;
  }

  uint8_t minIdx = 0;
  for (uint8_t i = 1; i < tempPeakCount; i++) {
    if (tempPeaks[i].tempF < tempPeaks[minIdx].tempF)
      minIdx = i;
  }

  if (tempF > tempPeaks[minIdx].tempF) {
    tempPeaks[minIdx] = { tempF, now };
  }
}

// Dew point (C) using Magnus formula
static inline float dewPointC(float tempC, float rh) {
  if (isnan(tempC) || isnan(rh) || rh <= 0.0f) return NAN;

  // Magnus constants for water over liquid range
  const float a = 17.62f;
  const float b = 243.12f;

  float gamma = logf(rh / 100.0f) + (a * tempC) / (b + tempC);
  float dp = (b * gamma) / (a - gamma);
  return dp;
}

String buildStatusJson() {
  String mode;
  bool apUp = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
  bool staUp = (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA);

  if (apUp && staUp) mode = "AP+STA";
  else if (apUp) mode = "AP";
  else if (staUp) mode = "STA";
  else mode = "OFF";

  String ipSta = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
  String ipAp = apUp ? WiFi.softAPIP().toString() : "";

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  time_t now;
  time(&now);

  auto peakForWindow = [&](uint32_t sec) -> float {
    float m = NAN;
    for (uint8_t i = 0; i < tempPeakCount; i++) {
      if ((now - tempPeaks[i].ts) <= sec) {
        if (isnan(m) || tempPeaks[i].tempF > m) m = tempPeaks[i].tempF;
      }
    }
    return m;
  };

  String j = "{";
  j += "\"ws\":\"CONNECTED\"";
  j += ",\"wifi\":\"" + wifiStatusString() + "\"";
  j += ",\"mode\":\"" + mode + "\"";
  j += ",\"ip_sta\":\"" + ipSta + "\"";
  j += ",\"ip_ap\":\"" + ipAp + "\"";
  j += ",\"mac\":\"" + String(macStr) + "\"";

  j += ",\"profile_id\":" + String((uint8_t)currentProfile);
  j += ",\"profile\":\"" + String(PROFILE_PRESETS[currentProfile].name) + "\"";

  j += ",\"tmin\":" + String(targetMinF, 2);
  j += ",\"tmax\":" + String(targetMaxF, 2);

  j += ",\"temp_f\":" + (isnan(lastTempF) ? "null" : String(lastTempF, 2));
  j += ",\"temp_c\":" + (isnan(lastTempC) ? "null" : String(lastTempC, 2));
  j += ",\"rh\":" + (isnan(lastRH) ? "null" : String(lastRH, 1));
  j += ",\"ah\":" + (isnan(lastAbsH) ? "null" : String(lastAbsH, 2));
  j += ",\"dew_f\":" + (isnan(lastDewF) ? "null" : String(lastDewF, 2));
  j += ",\"dew_c\":" + (isnan(lastDewC) ? "null" : String(lastDewC, 2));
  j += ",\"heat_f\":" + (isnan(lastHeatF) ? "null" : String(lastHeatF, 2));
  j += ",\"heat_c\":" + (isnan(lastHeatC) ? "null" : String(lastHeatC, 2));

  j += ",\"lamp\":" + String(relayOn ? "true" : "false");

  j += ",\"peak_1h\":" + (isnan(peakForWindow(3600)) ? "null" : String(peakForWindow(3600), 2));
  j += ",\"peak_3h\":" + (isnan(peakForWindow(3 * 3600)) ? "null" : String(peakForWindow(3 * 3600), 2));
  j += ",\"peak_6h\":" + (isnan(peakForWindow(6 * 3600)) ? "null" : String(peakForWindow(6 * 3600), 2));

  j += "}";
  return j;
}


// GET /api/wifi/scan  -> {"networks":[{"ssid":"x","rssi":-55,"enc":true}, ...]}
void handleApiWifiScanGet() {
  int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

  String j = "{";
  j += "\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i) j += ",";
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    bool enc = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

    j += "{";
    j += "\"ssid\":\"" + jsonEscape(ssid) + "\"";
    j += ",\"rssi\":" + String(rssi);
    j += ",\"enc\":" + String(enc ? "true" : "false");
    j += "}";
  }
  j += "]}";

  WiFi.scanDelete();
  server.send(200, "application/json", j);
}

void handleApiResetPost() {
  server.send(200, "application/json", "{\"ok\":true}");
  delay(200);
  ESP.restart();
}

void handleApiProfilePost() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  String body = server.arg("plain");
  int profileId;
  float tmin, tmax;

  if (!getJsonInt(body, "profile_id", profileId)) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  if (profileId < 0 || profileId > PROFILE_CUSTOM) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  // Custom requires temps
  if ((EggProfile)profileId == PROFILE_CUSTOM) {
    if (!getJsonFloat(body, "tmin", tmin) || !getJsonFloat(body, "tmax", tmax)) {
      server.send(400, "application/json", "{\"ok\":false}");
      return;
    }
  }

  applyProfile((EggProfile)profileId, tmin, tmax);
  wsBroadcastStatus();
  server.send(200, "application/json", "{\"ok\":true}");
}

void wsBroadcastStatus() {
  String msg = buildStatusJson();
  ws.broadcastTXT(msg);
}

const char STYLES[] PROGMEM = R"CSS(
:root {
  --bg:#0f1216;
  --panel:#161b22;
  --text:#e6edf3;
  --muted:#9da7b1;
  --accent:#4cc9f0;
  --ok:#3fb950;
  --bad:#f85149;
  --border:#2a2f36;
}
*{box-sizing:border-box}
body{
  margin:0;padding:16px;
  background:linear-gradient(180deg,#0f1216,#0b0e12);
  color:var(--text);
  font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;
}
.card{
  max-width:520px;margin:0 auto;
  background:var(--panel);
  border:1px solid var(--border);
  border-radius:12px;
  padding:16px;
  box-shadow:0 10px 30px rgba(0,0,0,.35);
}
h1{margin:0 0 12px;font-size:20px}
.row{
  display:flex;justify-content:space-between;align-items:center;
  padding:6px 0;border-bottom:1px solid var(--border)
}
.row:last-child{border-bottom:none}
.label{color:var(--muted);font-size:13px}
.value{font-weight:600}
.grid{
  display:grid;grid-template-columns:1fr 1fr;
  gap:10px;margin-top:10px
}
.metric{
  background:#11161c;border:1px solid var(--border);
  border-radius:10px;padding:12px;text-align:center
}
.metric .label{display:block;margin-bottom:6px}
.metric .value{font-size:20px;font-weight:800}
.metric.wide{
  grid-column:1 / -1;
  display:flex;
  justify-content:center;
  align-items:center;
  gap:10px;
  text-align:center;
  padding:12px;
}
.lampIcon{
  width:18px;height:18px;border-radius:50%;
  border:1px solid var(--border);
  box-shadow:0 0 18px rgba(76,201,240,.15);
  background:#0d1117;
}
.lampIcon.on{
  background: radial-gradient(circle at 30% 30%, #fff, #4cc9f0 40%, #0d1117 75%);
}
.lampText{font-weight:800}
select,input{
  width:85%;padding:8px;border-radius:8px;
  border:1px solid var(--border);
  background:#0d1117;color:var(--text)
}
.toast{
  position:fixed;bottom:20px;left:50%;
  transform:translateX(-50%);
  background:#0d1117;border:1px solid var(--border);
  padding:8px 14px;border-radius:20px;
  opacity:0;transition:.2s
}
.toast.show{opacity:1}
a{color:var(--accent);text-decoration:none;font-size:13px}
.mini{
  background:#0d1117;
  border:1px solid var(--border);
  border-radius:10px;
  padding:10px;
  margin-top:10px;
}
.miniTitle{
  display:flex;
  justify-content:space-between;
  align-items:center;
  margin-bottom:8px;
}
.miniTitle .t{
  font-size:13px;
  color:var(--muted);
  font-weight:600;
}
.peakList{
  display:flex;
  flex-direction:column;
  gap:6px;
  font-size:13px;
}
.peakRow{
  display:flex;
  justify-content:space-between;
  align-items:center;
  gap:10px;
  padding:6px 8px;
  border:1px solid var(--border);
  border-radius:10px;
  background:#11161c;
}
.peakTemp{font-weight:800}
.peakWhen{
  color:var(--muted);
  font-variant-numeric:tabular-nums;
  white-space:nowrap;
}
)CSS";

const char JAVASCRIPTS[] PROGMEM = R"JS(
const el = id => document.getElementById(id);

const uiState = {
  editingMin: false,
  editingMax: false,
  tmin: null,
  tmax: null,
};

let toast, tminEl, tmaxEl;
let ws = null;

function showToast(){
  toast.classList.add('show');
  setTimeout(()=>toast.classList.remove('show'),1000);
}

function fmt(v,d=2){
  if(v==null || Number.isNaN(v)) return '—';
  return Number(v).toFixed(d);
}

function pad2(n){return String(n).padStart(2,'0');}

function fmtWhen(ts){
  if(!ts) return '—';
  const d = new Date(ts*1000);
  return `${pad2(d.getHours())}:${pad2(d.getMinutes())}`;
}

function renderPeaks(list){
  const host = el('peaks');
  host.innerHTML = '';
  el('peakTop').textContent = '—';

  if(!Array.isArray(list) || !list.length){
    host.innerHTML =
      '<div class="peakRow"><span class="peakTemp">—</span><span class="peakWhen">No data</span></div>';
    return;
  }

  const sorted = list.slice().sort((a,b)=>a.ts-b.ts).slice(-6);
  el('peakTop').textContent = `${fmt(sorted[sorted.length-1].temp_f,1)}°F`;

  for(const p of sorted){
    const r = document.createElement('div');
    r.className = 'peakRow';
    r.innerHTML =
      `<span class="peakTemp">${fmt(p.temp_f,1)}°F</span>
       <span class="peakWhen">${fmtWhen(p.ts)}</span>`;
    host.appendChild(r);
  }
}

function applyStatus(s){
  if(!s) return;
  
  const wsEl = el('ws');
  if(wsEl) wsEl.textContent = s.ws || '—';
  const wifiEl = el('wifi');
  if(wifiEl) wifiEl.textContent = s.wifi || '—';
  const modeEl = el('mode');
  if(modeEl) modeEl.textContent = s.mode || '—';
  const ipApEl = el('ip_ap');
  if(ipApEl) ipApEl.textContent = s.ip_ap || '—';
  const ipStaEl = el('ip_sta');
  if(ipStaEl) ipStaEl.textContent = s.ip_sta || '—';
  const macEl = el('mac');
  if(macEl) macEl.textContent = s.mac || '—';

  const tempFEl = el('temp_f');
  if(tempFEl) tempFEl.textContent = fmt(s.temp_f);
  const tempCEl = el('temp_c');
  if(tempCEl) tempCEl.textContent = fmt(s.temp_c);
  const rhEl = el('rh');
  if(rhEl) rhEl.textContent = fmt(s.rh,1);
  const ahEl = el('ah');
  if(ahEl) ahEl.textContent = fmt(s.ah);
  const dewFEl = el('dew_f');
  if(dewFEl) dewFEl.textContent = fmt(s.dew_f);
  const heatFEl = el('heat_f');
  if(heatFEl) heatFEl.textContent = fmt(s.heat_f);

  const lampOn = !!s.lamp;
  const lampEl = el('lamp');
  if(lampEl) lampEl.textContent = lampOn ? 'ON' : 'OFF';
  const lampIconEl = el('lampIcon');
  if(lampIconEl) lampIconEl.classList.toggle('on', lampOn);

  const isCustom = (s.profile_id === 5);

  const rangeMinEl = el('range_min');
  if(rangeMinEl) rangeMinEl.textContent = fmt(s.tmin,1);
  const rangeMaxEl = el('range_max');
  if(rangeMaxEl) rangeMaxEl.textContent = fmt(s.tmax,1);

  const customRangeEl = el('customRange');
  if(customRangeEl) customRangeEl.style.display = isCustom ? 'block' : 'none';
  const rangeDisplayEl = el('rangeDisplay');
  if(rangeDisplayEl) rangeDisplayEl.style.display = isCustom ? 'none' : 'flex';

  uiState.tmin = s.tmin;
  uiState.tmax = s.tmax;

  if(isCustom && tminEl && tmaxEl){
    if(!uiState.editingMin) tminEl.value = fmt(s.tmin,1);
    if(!uiState.editingMax) tmaxEl.value = fmt(s.tmax,1);
  }

  renderPeaks(s.temp_peaks || []);
}

function saveCustomRange(){
  if(uiState.tmin == null || uiState.tmax == null) return;
  fetch('/api/profile',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({
      profile_id:5,
      tmin:uiState.tmin,
      tmax:uiState.tmax
    })
  }).then(showToast);
}

function resetDevice(){
  if(confirm('This will reboot the device')){
    fetch('/api/reset',{method:'POST'});
  }
}

function connectWebSocket(){
  if(ws && ws.readyState === WebSocket.OPEN) return;
  
  try {
    ws = new WebSocket(`ws://${location.hostname}:81/`);
    
    ws.onopen = () => {
      console.log('WebSocket connected');
      if(el('ws')) el('ws').textContent = 'CONNECTED';
    };
    
    ws.onclose = () => {
      console.log('WebSocket disconnected');
      if(el('ws')) el('ws').textContent = 'DISCONNECTED';
      setTimeout(connectWebSocket, 2000);
    };
    
    ws.onerror = (err) => {
      console.error('WebSocket error:', err);
      if(el('ws')) el('ws').textContent = 'ERROR';
    };
    
    ws.onmessage = (e) => {
      try {
        const data = JSON.parse(e.data);
        applyStatus(data);
      } catch(err) {
        console.error('Failed to parse WebSocket message:', err, e.data);
      }
    };
  } catch(err) {
    console.error('Failed to create WebSocket:', err);
    if(el('ws')) el('ws').textContent = 'ERROR';
    setTimeout(connectWebSocket, 2000);
  }
}

document.addEventListener('DOMContentLoaded', () => {
  toast = el('toast');
  tminEl = el('tmin');
  tmaxEl = el('tmax');
  
  if(tminEl) tminEl.onfocus = () => uiState.editingMin = true;
  if(tmaxEl) tmaxEl.onfocus = () => uiState.editingMax = true;
  
  if(tminEl) tminEl.onblur = () => {
    uiState.editingMin = false;
    uiState.tmin = parseFloat(tminEl.value);
    saveCustomRange();
  };
  
  if(tmaxEl) tmaxEl.onblur = () => {
    uiState.editingMax = false;
    uiState.tmax = parseFloat(tmaxEl.value);
    saveCustomRange();
  };
  
  connectWebSocket();
});
)JS";

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Incubator</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="card">
  <h1>🥚 Incubator</h1>

  <!-- System -->
  <div class="row"><span class="label">WiFi</span><span id="wifi" class="value">—</span></div>
  <div class="row"><span class="label">WebSocket</span><span id="ws" class="value">—</span></div>
  <div class="row"><span class="label">Mode</span><span id="mode" class="value">—</span></div>
  <div class="row"><span class="label">STA IP</span><span id="ip_sta" class="value">—</span></div>
  <div class="row"><span class="label">AP IP</span><span id="ip_ap" class="value">—</span></div>
  <div class="row"><span class="label">MAC</span><span id="mac" class="value">—</span></div>

  <div style="text-align:right;margin-top:8px;margin-bottom:6px">
    <a href="/wifi">WiFi Settings</a><br>
    <a href="#" onclick="resetDevice(); return false;" style="margin-top:4px;display:inline-block">Reboot</a>
  </div>

  <!-- Metrics -->
  <div class="grid">
    <div class="metric"><span class="label">Temp (°F)</span><span id="temp_f" class="value">—</span></div>
    <div class="metric"><span class="label">Temp (°C)</span><span id="temp_c" class="value">—</span></div>
    <div class="metric"><span class="label">Humidity (%)</span><span id="rh" class="value">—</span></div>
    <div class="metric"><span class="label">Abs Hum (g/m³)</span><span id="ah" class="value">—</span></div>
    <div class="metric"><span class="label">Dew (°F)</span><span id="dew_f" class="value">—</span></div>
    <div class="metric"><span class="label">HeatIdx (°F)</span><span id="heat_f" class="value">—</span></div>

    <div class="metric wide">
      <div id="lampIcon" class="lampIcon"></div>
      <div class="lampText">Lamp <span id="lamp">—</span></div>
    </div>
  </div>

  <!-- Temperature Range -->
  <div class="mini">
    <div class="miniTitle"><span class="t">Temperature Range</span></div>

    <div id="rangeDisplay" class="row">
      <span class="label">Configured</span>
      <span class="value">
        <span id="range_min">—</span> – <span id="range_max">—</span> °F
      </span>
    </div>

    <div id="customRange" style="display:none;margin-top:8px">
      <input id="tmin" placeholder="Min °F"/>
      <input id="tmax" placeholder="Max °F" style="margin-top:6px"/>
    </div>
  </div>

  <!-- Peaks -->
  <div class="mini">
    <div class="miniTitle">
      <span class="t">Top 6 Temp Peaks</span>
      <span id="peakTop" class="value">—</span>
    </div>
    <div id="peaks" class="peakList"></div>
  </div>
</div>

<div id="toast" class="toast">Saved</div>

<script src="/app.js"></script>
</body>
</html>
)HTML";

const char WIFI_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>WiFi Settings</title>
  <style>
    :root {
      --bg:#0f1216;
      --panel:#161b22;
      --text:#e6edf3;
      --muted:#9da7b1;
      --accent:#4cc9f0;
      --border:#2a2f36;
      --ok:#3fb950;
    }
    *{box-sizing:border-box}
    body{
      margin:0;padding:16px;
      background:linear-gradient(180deg,#0f1216,#0b0e12);
      color:var(--text);
      font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;
    }
    .card{
      max-width:520px;margin:0 auto;
      background:var(--panel);
      border:1px solid var(--border);
      border-radius:12px;
      padding:16px;
      box-shadow:0 10px 30px rgba(0,0,0,.35);
    }
    h1{margin:0 0 12px;font-size:20px}
    .row{margin-bottom:10px}
    label{font-size:13px;color:var(--muted);display:block;margin-bottom:4px}
    input,select{
      width:100%;
      padding:8px;
      border-radius:8px;
      border:1px solid var(--border);
      background:#0d1117;
      color:var(--text)
    }
    button{
      width:100%;
      padding:10px;
      border-radius:10px;
      border:none;
      font-weight:700;
      background:var(--accent);
      color:#000;
      cursor:pointer;
    }
    .net{
      padding:8px;
      border:1px solid var(--border);
      border-radius:8px;
      margin-bottom:6px;
      cursor:pointer;
      background:#11161c;
      font-size:13px;
    }
    .net small{color:var(--muted)}
    a{color:var(--accent);font-size:13px;text-decoration:none}
  </style>
</head>
<body>
<div class="card">
  <h1>📡 WiFi Settings</h1>

  <div class="row">
    <label>SSID</label>
    <input id="ssid"/>
  </div>

  <div class="row">
    <label>Password</label>
    <input id="pass" type="password"/>
  </div>

  <div class="row">
    <label>
      <input type="checkbox" id="keep_ap"/> Keep Access Point enabled
    </label>
  </div>

  <button onclick="save()">Save & Apply</button>

  <div class="row" style="margin-top:14px">
    <label>Available Networks</label>
    <div id="nets"></div>
  </div>

  <div style="text-align:right;margin-top:8px">
    <a href="/">← Back to Status</a>
  </div>
</div>

<script>
const el=id=>document.getElementById(id);

function load(){
  fetch('/api/wifi').then(r=>r.json()).then(j=>{
    el('ssid').value=j.ssid||'';
    el('keep_ap').checked=!!j.keep_ap;
  });

  fetch('/api/wifi/scan').then(r=>r.json()).then(j=>{
    const host=el('nets');
    host.innerHTML='';
    j.networks.forEach(n=>{
      const d=document.createElement('div');
      d.className='net';
      d.innerHTML=`<b>${n.ssid||'(hidden)'}</b><br><small>RSSI ${n.rssi} ${n.enc?'🔒':''}</small>`;
      d.onclick=()=>el('ssid').value=n.ssid;
      host.appendChild(d);
    });
  });
}

function save(){
  fetch('/api/wifi',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({
      ssid:el('ssid').value,
      pass:el('pass').value,
      keep_ap:el('keep_ap').checked
    })
  }).then(()=>alert('Saved. Reconnecting…'));
}

load();
</script>
</body>
</html>
)HTML";


void loadPrefs() {
  prefs.begin("incubator", false);

  // 1. Defaults (always start Chicken)
  currentProfile = PROFILE_CHICKEN;
  targetMinF = PROFILE_PRESETS[PROFILE_CHICKEN].tmin;
  targetMaxF = PROFILE_PRESETS[PROFILE_CHICKEN].tmax;

  // 2. Load stored profile if exists
  if (prefs.isKey("profile")) {
    uint8_t p = prefs.getUChar("profile", PROFILE_CHICKEN);
    if (p <= PROFILE_CUSTOM) {
      currentProfile = (EggProfile)p;
    }
  }

  // 3. Load temps if stored
  bool hasTemps = prefs.isKey("tmin") && prefs.isKey("tmax");
  if (hasTemps) {
    targetMinF = prefs.getFloat("tmin");
    targetMaxF = prefs.getFloat("tmax");
  }

  // 4. Apply preset overwrite unless CUSTOM
  if (currentProfile != PROFILE_CUSTOM) {
    targetMinF = PROFILE_PRESETS[currentProfile].tmin;
    targetMaxF = PROFILE_PRESETS[currentProfile].tmax;
  }

  // 5. WiFi
  staSsid = prefs.getString("ssid", "");
  staPass = prefs.getString("pass", "");
  keepApWhenConnected = prefs.getBool("keepap", true);
}

void applyProfile(EggProfile p, float tmin, float tmax) {
  currentProfile = p;

  if (p == PROFILE_CUSTOM) {
    targetMinF = tmin;
    targetMaxF = tmax;
  } else {
    targetMinF = PROFILE_PRESETS[p].tmin;
    targetMaxF = PROFILE_PRESETS[p].tmax;
  }

  prefs.putUChar("profile", (uint8_t)p);
  prefs.putFloat("tmin", targetMinF);
  prefs.putFloat("tmax", targetMaxF);
}

void saveTargets(float tmin, float tmax) {
  targetMinF = tmin;
  targetMaxF = tmax;
  prefs.putFloat("tmin", targetMinF);
  prefs.putFloat("tmax", targetMaxF);
}

void saveWifi(const String& ssid, const String& pass, bool keepAp) {
  staSsid = ssid;
  staPass = pass;
  keepApWhenConnected = keepAp;

  prefs.putString("ssid", staSsid);
  prefs.putString("pass", staPass);
  prefs.putBool("keepap", keepApWhenConnected);
}

void startAP() {
  // Ensure AP interface hostname + IP are what you want before bringing AP up
  WiFi.softAPdisconnect(true);

  // AP IP must be 10.0.0.1
  IPAddress apIP(10, 0, 0, 1);
  IPAddress gw(10, 0, 0, 1);
  IPAddress mask(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, gw, mask);

  // DHCP lease "device name" on the AP side (clients connected to Incubator AP)
  WiFi.softAPsetHostname("Incubator");

  // Bring up AP
  WiFi.softAP(AP_SSID, AP_PASS);
}

bool connectSTA(uint32_t timeoutMs = 15000) {
  if (staSsid.length() == 0) return false;

  // Set static IP
  IPAddress staticIP(192, 168, 10, 44);
  IPAddress gateway(192, 168, 10, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 10, 1);
  
  if (!WiFi.config(staticIP, gateway, subnet, dns)) {
    Serial.println("Failed to configure static IP");
  }

  WiFi.begin(staSsid.c_str(), staPass.c_str());
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) return true;
    delay(100);
  }
  return (WiFi.status() == WL_CONNECTED);
}

void applyWifiMode() {
  bool haveCreds = (staSsid.length() > 0);

  if (!haveCreds) {
    WiFi.mode(WIFI_AP);
    startAP();
    return;
  }

  // Start AP+STA so we can always reach it during connect attempt
  WiFi.mode(WIFI_AP_STA);
  startAP();

  bool ok = connectSTA(15000);

  if (ok) {
    if (!keepApWhenConnected) {
      // STA only
      WiFi.mode(WIFI_STA);
    } else {
      // Keep AP up
      WiFi.mode(WIFI_AP_STA);
      // Re-assert AP config/hostname in case mode switch disturbed it
      startAP();
    }
  } else {
    // Stay AP+STA (AP is up, STA will keep trying in background)
    WiFi.mode(WIFI_AP_STA);
  }
}

void setupOTA() {
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.setHostname("Incubator");
  ArduinoOTA.begin();
}

void handleStyleCss() {
  server.send_P(200, "text/css", STYLES);
}

void handleJavascript() {
  server.send_P(200, "application/javascript", JAVASCRIPTS);
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleWifiPage() {
  server.send_P(200, "text/html", WIFI_HTML);
}

// Very small JSON extractor for {"tmin":x,"tmax":y} etc (no extra libs)
bool getJsonFloat(const String& body, const char* key, float& out) {
  String k = String("\"") + key + "\"";
  int i = body.indexOf(k);
  if (i < 0) return false;
  i = body.indexOf(':', i);
  if (i < 0) return false;
  int j = i + 1;
  while (j < (int)body.length() && (body[j] == ' ')) j++;
  int end = j;
  while (end < (int)body.length() && (isDigit(body[end]) || body[end] == '-' || body[end] == '.')) end++;
  if (end == j) return false;
  out = body.substring(j, end).toFloat();
  return true;
}

bool getJsonBool(const String& body, const char* key, bool& out) {
  String k = String("\"") + key + "\"";
  int i = body.indexOf(k);
  if (i < 0) return false;
  i = body.indexOf(':', i);
  if (i < 0) return false;
  int j = i + 1;
  while (j < (int)body.length() && (body[j] == ' ')) j++;
  if (body.startsWith("true", j)) {
    out = true;
    return true;
  }
  if (body.startsWith("false", j)) {
    out = false;
    return true;
  }
  return false;
}

bool getJsonString(const String& body, const char* key, String& out) {
  String k = String("\"") + key + "\"";
  int i = body.indexOf(k);
  if (i < 0) return false;
  i = body.indexOf(':', i);
  if (i < 0) return false;
  int j = body.indexOf('\"', i + 1);
  if (j < 0) return false;
  int end = body.indexOf('\"', j + 1);
  if (end < 0) return false;
  out = body.substring(j + 1, end);
  return true;
}

void handleApiRangePost() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Missing body\"}");
    return;
  }

  String body = server.arg("plain");
  float tmin, tmax;
  if (!getJsonFloat(body, "tmin", tmin) || !getJsonFloat(body, "tmax", tmax)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  if (tmin < TEMP_MIN_ALLOWED_F || tmin > TEMP_MAX_ALLOWED_F || tmax < TEMP_MIN_ALLOWED_F || tmax > TEMP_MAX_ALLOWED_F) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Out of allowed range\"}");
    return;
  }
  if (tmin >= tmax) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Min must be < Max\"}");
    return;
  }

  saveTargets(tmin, tmax);
  wsBroadcastStatus();

  server.send(200, "application/json", "{\"ok\":true}");
}

void handleApiWifiGet() {
  String j = "{";
  j += "\"ssid\":\"" + jsonEscape(staSsid) + "\"";
  j += ",\"keep_ap\":" + String(keepApWhenConnected ? "true" : "false");
  j += "}";
  server.send(200, "application/json", j);
}

void handleApiWifiPost() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Missing body\"}");
    return;
  }

  String body = server.arg("plain");
  String ssid, pass;
  bool keepAp = keepApWhenConnected;

  if (!getJsonString(body, "ssid", ssid)) ssid = "";
  if (!getJsonString(body, "pass", pass)) pass = "";
  getJsonBool(body, "keep_ap", keepAp);

  saveWifi(ssid, pass, keepAp);

  WiFi.setHostname("Incubator");

  // Apply immediately
  applyWifiMode();
  wsBroadcastStatus();

  server.send(200, "application/json", "{\"ok\":true}");
}

bool getJsonInt(const String& body, const char* key, int& out) {
  float f;
  if (!getJsonFloat(body, key, f)) return false;
  out = (int)f;
  return true;
}

void setupHttpRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/wifi", HTTP_GET, handleWifiPage);
  server.on("/style.css", HTTP_GET, handleStyleCss);
  server.on("/app.js", HTTP_GET, handleJavascript);

  server.on("/api/range", HTTP_POST, handleApiRangePost);
  server.on("/api/profile", HTTP_POST, handleApiProfilePost);

  server.on("/api/wifi", HTTP_GET, handleApiWifiGet);
  server.on("/api/wifi", HTTP_POST, handleApiWifiPost);
  server.on("/api/wifi/scan", HTTP_GET, handleApiWifiScanGet);

  server.on("/api/reset", HTTP_POST, handleApiResetPost);

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
}


void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  (void)payload;
  (void)length;
  if (type == WStype_CONNECTED) {
    // Send current status immediately to the new client
    String msg = buildStatusJson();
    ws.sendTXT(num, msg);
  }
}

void lockStaMac() {
  uint64_t mac64 = ESP.getEfuseMac();  // 48-bit base MAC in lower bits
  uint8_t mac[6];

  mac[0] = (mac64 >> 40) & 0xFF;
  mac[1] = (mac64 >> 32) & 0xFF;
  mac[2] = (mac64 >> 24) & 0xFF;
  mac[3] = (mac64 >> 16) & 0xFF;
  mac[4] = (mac64 >> 8)  & 0xFF;
  mac[5] = mac64 & 0xFF;

  esp_wifi_set_mac(WIFI_IF_STA, mac);
}


void setup() {
  Serial.begin(115200);
  lockStaMac();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Start with lamp OFF until we get valid sensor reading
  relayOn = false;
  lastRelayOn = relayOn;

  loadPrefs();  // Load preferences BEFORE sensor reads so targetMinF/targetMaxF are set correctly

  dht.begin();
  delay(2500); // DHT22 needs time to stabilize

  // Try to read sensor immediately after initialization (before WiFi setup)
  // This ensures no interference from WiFi initialization
  bool sensorRead = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    float tempC = dht.readTemperature();
    delay(100);
    float hum = dht.readHumidity();
    
    if (!isnan(tempC) && !isnan(hum) && tempC > -50.0f && tempC < 100.0f && hum >= 0.0f && hum <= 100.0f) {
      lastTempC = tempC;
      lastTempF = cToF(tempC);
      lastRH = hum;
      lastAbsH = absoluteHumidity_gm3(tempC, hum);
      
      lastDewC = dewPointC(lastTempC, lastRH);
      lastDewF = isnan(lastDewC) ? NAN : cToF(lastDewC);
      
      lastHeatF = dht.computeHeatIndex(lastTempF, lastRH, true);
      lastHeatC = isnan(lastHeatF) ? NAN : ((lastHeatF - 32.0f) * 5.0f / 9.0f);
      
      recordTempPeak(lastTempF);
      
      if (lastTempF <= targetMinF) relayOn = true;
      else if (lastTempF >= targetMaxF) relayOn = false;
      
      digitalWrite(RELAY_PIN, relayOn ? HIGH : LOW);
      sensorRead = true;
      break; // Success, exit retry loop
    }
    
    // Wait before retry (DHT22 needs time between reads)
    if (attempt < 2) delay(2000);
  }
  
  // If sensor read failed, ensure lamp is OFF and values are null
  if (!sensorRead) {
    relayOn = false;
    digitalWrite(RELAY_PIN, LOW);
    lastTempC = NAN;
    lastTempF = NAN;
    lastRH = NAN;
    lastAbsH = NAN;
    lastDewC = NAN;
    lastDewF = NAN;
    lastHeatC = NAN;
    lastHeatF = NAN;
  }

  WiFi.setSleep(false);
  applyWifiMode();

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  setupOTA();

  ws.begin();
  ws.onEvent(onWsEvent);

  setupHttpRoutes();
  
  lastSensorMs = millis();
  wsBroadcastStatus();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  ws.loop();

  uint32_t now = millis();
  if (now - lastSensorMs < SENSOR_INTERVAL_MS) return;
  lastSensorMs = now;

  // Read sensor with validation
  // DHT library: readTemperature() triggers sensor read and caches both temp and humidity
  // On ESP32-S3, we need to ensure proper timing between reads
  float tempC = dht.readTemperature();
  delay(50); // Small delay to ensure sensor read completes on ESP32-S3
  float hum = dht.readHumidity();

  // Validate readings are within reasonable ranges
  if (!isnan(tempC) && !isnan(hum) && 
      tempC > -50.0f && tempC < 100.0f && 
      hum >= 0.0f && hum <= 100.0f) {
    // Valid sensor reading
    lastTempC = tempC;
    lastTempF = cToF(tempC);
    lastRH = hum;
    lastAbsH = absoluteHumidity_gm3(tempC, hum);

    lastDewC = dewPointC(lastTempC, lastRH);
    lastDewF = isnan(lastDewC) ? NAN : cToF(lastDewC);

    lastHeatF = dht.computeHeatIndex(lastTempF, lastRH, true);
    lastHeatC = isnan(lastHeatF) ? NAN : ((lastHeatF - 32.0f) * 5.0f / 9.0f);

    recordTempPeak(lastTempF);

    lastRelayOn = relayOn;
    if (lastTempF <= targetMinF) relayOn = true;
    else if (lastTempF >= targetMaxF) relayOn = false;

    digitalWrite(RELAY_PIN, relayOn ? HIGH : LOW);
    wsBroadcastStatus();
  } else {
    // SENSOR DEAD → LAMP OFF (safety: don't heat if we can't read temp)
    relayOn = false;
    digitalWrite(RELAY_PIN, LOW);
    
    // Always clear sensor values when read fails
    lastTempC = NAN;
    lastTempF = NAN;
    lastRH = NAN;
    lastAbsH = NAN;
    lastDewC = NAN;
    lastDewF = NAN;
    lastHeatC = NAN;
    lastHeatF = NAN;
    
    // Broadcast status when sensor fails (so UI shows null and lamp OFF)
    static uint32_t lastDeadBroadcast = 0;
    if (now - lastDeadBroadcast > 2000) { // Broadcast every 2 seconds when dead
      lastDeadBroadcast = now;
      wsBroadcastStatus();
    }
  }
}
