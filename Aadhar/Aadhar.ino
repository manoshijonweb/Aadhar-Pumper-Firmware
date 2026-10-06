// Aadhar - mosquito larvae guard for potholes and standing water
//
// The device stands in places where water collects. When the water goes still,
// the pump ripples it so mosquito larvae can't develop.
//
// Probes: GPIO 32 (top), 33 (middle), 34 (bottom) - ADC1 only, ADC2 stops with WiFi on
// Relay:  GPIO 27 (active HIGH: HIGH = pump on, see RELAY_ON)
//
// Every check (default 3 h) the probes are read. If any probe shows water at a
// level that hasn't changed since the last check, the pump runs (default
// 15 min). If all probes are full, the pump runs at most once every 6 h.
// Thresholds and timings can be changed from the dashboard.
//
// Dashboard: the ESP32 always runs its own hotspot "Aadhar" (password in secrets.h).
// Join it and open http://192.168.4.1 - no router needed. If the saved WiFi is in
// range it joins that too, and the dashboard is also at http://aadhar.local there.
// The saved WiFi can be changed from the dashboard.
//
// Passwords live in secrets.h (not in git): copy secrets.example.h to secrets.h.

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "dashboard.h"
#include "secrets.h"

// ---------- Fixed settings ----------
const char *DEFAULT_SSID = WIFI_SSID, *DEFAULT_PASS = WIFI_PASS;  // until changed from dashboard
const char *AP_SSID = "Aadhar", *AP_PASS = HOTSPOT_PASS;          // hotspot, always on
const char *HOSTNAME = "aadhar";

const int SENSORS[3] = {32, 33, 34};
const int RELAY = 27;
const int RELAY_ON = HIGH;  // this relay module switches on with HIGH; use LOW for active-low modules

const unsigned long MINUTE = 60000UL, HOUR = 60 * MINUTE;
const unsigned long LIVE_TIME = 1000;         // live reading interval
const unsigned long HISTORY_TIME = 10000;     // chart history sample interval
const int HISTORY_SIZE = 360;                 // 360 x 10 s = 1 hour
const unsigned long WIFI_TIMEOUT = 15000;     // wait for WiFi at boot
const unsigned long WIFI_RETRY_TIME = 5 * MINUTE;  // offline: retry the saved WiFi this often
                                                   // (each try briefly disturbs the hotspot)

// ---------- Adjustable settings (saved from dashboard) ----------
struct Settings {
  int noWater = 128;     // at or below: dry
  int full = 3967;       // at or above: full
  int stable = 128;      // max change between checks to count as stable
  int checkMin = 180;    // minutes between checks
  int pumpMin = 15;      // pump run time, minutes
  int fullHours = 6;     // pump at most this often when full
} cfg;

// ---------- State ----------
WebServer server(80);
Preferences prefs;
String ssid, pass;

int live[3], current[3], previous[3];
bool firstReading = true, checkRequested = false, pumpOn = false;
bool wifiUp = false, restartPending = false;
unsigned long lastCheck, lastLive, lastHistory, lastFullPump, pumpStart, pumpTime, lastWifiTry, restartAt;
int pumpRuns = 0;                 // since boot, for the dashboard
String checkResult = "none";      // last check: none, dry, moving, still, full

int16_t history[HISTORY_SIZE][3];
int historyCount = 0, historyHead = 0;

// ---------- Event log (shown on dashboard) ----------
const int MAX_EVENTS = 30;
struct Event { unsigned long time; String text; } events[MAX_EVENTS];
int eventCount = 0, eventHead = 0;

void logEvent(const String &text) {
  Serial.println(text);
  events[eventHead] = {millis(), text};
  eventHead = (eventHead + 1) % MAX_EVENTS;
  if (eventCount < MAX_EVENTS) eventCount++;
}

String readings(const int v[3]) {
  return "[" + String(v[0]) + "," + String(v[1]) + "," + String(v[2]) + "]";
}

String jsonText(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  return "\"" + s + "\"";
}

bool hasWater(int v) { return v > cfg.noWater && v < cfg.full; }

// Probes in water are noisy: average 32 samples, dropping the highest and lowest.
int readProbe(int pin) {
  long sum = 0;
  int low = 4095, high = 0;
  for (int i = 0; i < 32; i++) {
    int v = analogRead(pin);
    sum += v;
    low = min(low, v);
    high = max(high, v);
  }
  return (sum - low - high) / 30;
}

// ---------- Pump & sensor check ----------
void setPump(bool on, const String &reason, int minutes = 0) {
  digitalWrite(RELAY, on ? RELAY_ON : !RELAY_ON);
  pumpOn = on;
  if (on) {
    pumpStart = millis();
    pumpTime = (minutes > 0 ? minutes : cfg.pumpMin) * MINUTE;
    pumpRuns++;
    logEvent("PUMP ON - " + String(pumpTime / MINUTE) + " min (" + reason + ").");
  } else {
    logEvent("PUMP OFF (" + reason + ").");
  }
}

void runCheck(unsigned long now) {
  for (int i = 0; i < 3; i++) current[i] = readProbe(SENSORS[i]);
  String msg = "Check " + readings(current) + ": ";

  if (firstReading) {
    firstReading = false;
    lastFullPump = now;
    logEvent("Initial readings stored: " + readings(current));
  } else if (current[0] >= cfg.full && current[1] >= cfg.full && current[2] >= cfg.full) {
    checkResult = "full";
    if (now - lastFullPump >= cfg.fullHours * HOUR) {
      logEvent(msg + "maximum level for " + String(cfg.fullHours) + " hours.");
      setPump(true, "maximum level");
      lastFullPump = now;
    } else {
      logEvent(msg + "maximum level maintained.");
    }
  } else {
    bool stable = false, anyWet = false;
    for (int i = 0; i < 3; i++) {
      if (current[i] > cfg.noWater) anyWet = true;
      if (hasWater(current[i]) && hasWater(previous[i]) &&
          abs(current[i] - previous[i]) <= cfg.stable) stable = true;
    }
    checkResult = stable ? "still" : anyWet ? "moving" : "dry";
    logEvent(msg + (stable ? "still water detected." : anyWet ? "water level changing." : "no water."));
    if (stable) setPump(true, "still water");
  }

  memcpy(previous, current, sizeof(current));
}

// ---------- WiFi ----------
// The hotspot stays on all the time. The saved WiFi is optional: while it's out of
// range, retry only every few minutes - constant retries make the hotspot drop out.
void checkWifi(unsigned long now) {
  bool up = WiFi.status() == WL_CONNECTED;
  if (up && !wifiUp) {
    logEvent("WiFi connected to " + ssid + " - dashboard also at http://" + WiFi.localIP().toString());
  } else if (!up && wifiUp) {
    logEvent("WiFi lost - hotspot still on, retrying.");
    lastWifiTry = now - WIFI_RETRY_TIME;  // retry straight away once
  }
  wifiUp = up;
  if (!up && now - lastWifiTry >= WIFI_RETRY_TIME) {
    lastWifiTry = now;
    WiFi.reconnect();
  }
}

void startWifi() {
  WiFi.setHostname(HOSTNAME);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  logEvent("Hotspot " + String(AP_SSID) + " on - dashboard at http://" + WiFi.softAPIP().toString());

  WiFi.setAutoReconnect(false);  // checkWifi retries instead
  WiFi.begin(ssid.c_str(), pass.c_str());
  lastWifiTry = millis();

  Serial.print("Connecting to WiFi " + ssid);
  for (unsigned long t = millis(); WiFi.status() != WL_CONNECTED && millis() - t < WIFI_TIMEOUT; delay(500)) {
    Serial.print(".");
  }
  Serial.println();

  checkWifi(millis());
  if (!wifiUp) {
    // Pump control and the hotspot dashboard keep running without it.
    logEvent("Could not connect to " + ssid + " - retrying every " + String(WIFI_RETRY_TIME / MINUTE) + " min.");
  }
  MDNS.begin(HOSTNAME);
}

// ---------- Saved settings ----------
void loadSettings() {
  prefs.begin("wifi");
  ssid = prefs.isKey("ssid") ? prefs.getString("ssid") : DEFAULT_SSID;
  pass = prefs.isKey("pass") ? prefs.getString("pass") : DEFAULT_PASS;
  if (prefs.isKey("cfg") && prefs.getBytesLength("cfg") == sizeof(cfg)) prefs.getBytes("cfg", &cfg, sizeof(cfg));
  prefs.end();
}

// Reads one setting from the request; false if missing or out of range.
bool readSetting(const char *name, int &value, int low, int high) {
  if (!server.hasArg(name)) return false;
  int v = server.arg(name).toInt();
  if (v < low || v > high) return false;
  value = v;
  return true;
}

// ---------- Web server ----------
void sendOk() { server.send(200, "application/json", "{\"ok\":true}"); }
void sendError(const String &msg) { server.send(400, "application/json", "{\"ok\":false,\"error\":" + jsonText(msg) + "}"); }

void handleStatus() {
  unsigned long now = millis();
  unsigned long checkTime = cfg.checkMin * MINUTE;
  unsigned long pumpLeft = pumpOn && now - pumpStart < pumpTime ? pumpTime - (now - pumpStart) : 0;
  unsigned long nextCheck = !firstReading && now - lastCheck < checkTime ? checkTime - (now - lastCheck) : 0;

  String json = "{\"uptime\":" + String(now) +
                ",\"pins\":" + readings(SENSORS) +
                ",\"values\":" + readings(live) +
                ",\"pump\":" + (pumpOn ? "true" : "false") +
                ",\"pumpLeft\":" + String(pumpLeft) +
                ",\"pumpTotal\":" + String(pumpOn ? pumpTime : 0) +
                ",\"pumpRuns\":" + String(pumpRuns) +
                ",\"lastPump\":" + String(pumpRuns ? pumpStart : 0) +
                ",\"checkResult\":" + jsonText(checkResult) +
                ",\"nextCheck\":" + String(nextCheck) +
                ",\"checkEvery\":" + String(checkTime) +
                ",\"cfg\":{\"noWater\":" + String(cfg.noWater) +
                ",\"full\":" + String(cfg.full) +
                ",\"stable\":" + String(cfg.stable) +
                ",\"checkMin\":" + String(cfg.checkMin) +
                ",\"pumpMin\":" + String(cfg.pumpMin) +
                ",\"fullHours\":" + String(cfg.fullHours) + "}" +
                ",\"rssi\":" + String(wifiUp ? WiFi.RSSI() : 0) +
                ",\"ssid\":" + jsonText(ssid) +
                ",\"connected\":" + (wifiUp ? "true" : "false") +
                ",\"ip\":" + jsonText(WiFi.localIP().toString()) +
                ",\"apSsid\":" + jsonText(AP_SSID) +
                ",\"apIp\":" + jsonText(WiFi.softAPIP().toString()) +
                ",\"events\":[";

  for (int i = 0; i < eventCount; i++) {  // oldest first
    Event &e = events[(eventHead - eventCount + i + MAX_EVENTS) % MAX_EVENTS];
    json += String(i ? "," : "") + "{\"t\":" + String(e.time) + ",\"m\":" + jsonText(e.text) + "}";
  }
  server.send(200, "application/json", json + "]}");
}

void handleHistory() {
  String json;
  json.reserve(historyCount * 18 + 80);
  json = "{\"last\":" + String(lastHistory) + ",\"every\":" + String(HISTORY_TIME) + ",\"data\":[";
  for (int i = 0; i < historyCount; i++) {  // oldest first
    int16_t *h = history[(historyHead - historyCount + i + HISTORY_SIZE) % HISTORY_SIZE];
    json += String(i ? "," : "") + "[" + String(h[0]) + "," + String(h[1]) + "," + String(h[2]) + "]";
  }
  server.send(200, "application/json", json + "]}");
}

void startServer() {
  server.on("/", [] { server.send(200, "text/html", DASHBOARD_HTML); });
  server.on("/api/status", handleStatus);
  server.on("/api/history", handleHistory);

  server.on("/api/pump", HTTP_POST, [] {  // ?state=on|off&min=N
    String state = server.arg("state");
    if (state == "on" && !pumpOn) setPump(true, "manual", constrain(server.arg("min").toInt(), 0, 120));
    if (state == "off" && pumpOn) setPump(false, "manual");
    sendOk();
  });

  server.on("/api/check", HTTP_POST, [] {  // ignored while the pump runs
    if (!pumpOn) {
      checkRequested = true;
      logEvent("Manual check requested.");
    }
    sendOk();
  });

  server.on("/api/settings", HTTP_POST, [] {
    Settings s = cfg;
    if (!readSetting("noWater", s.noWater, 0, 4095) || !readSetting("full", s.full, 1, 4095) ||
        !readSetting("stable", s.stable, 1, 4095) || !readSetting("checkMin", s.checkMin, 1, 1440) ||
        !readSetting("pumpMin", s.pumpMin, 1, 120) || !readSetting("fullHours", s.fullHours, 1, 72)) {
      return sendError("A value is missing or out of range.");
    }
    if (s.noWater >= s.full) return sendError("Dry level must be below the full level.");
    cfg = s;
    prefs.begin("wifi");
    prefs.putBytes("cfg", &cfg, sizeof(cfg));
    prefs.end();
    logEvent("Settings updated.");
    sendOk();
  });

  server.on("/api/wifi", HTTP_POST, [] {  // ssid, pass -> save and restart
    String newSsid = server.arg("ssid");
    newSsid.trim();
    if (newSsid.isEmpty()) return sendError("WiFi name is empty.");
    prefs.begin("wifi");
    prefs.putString("ssid", newSsid);
    prefs.putString("pass", server.arg("pass"));
    prefs.end();
    logEvent("WiFi changed to " + newSsid + " - restarting.");
    sendOk();
    restartPending = true;  // restart once the reply has gone out
    restartAt = millis();
  });

  server.begin();
}

// ---------- Main ----------
void setup() {
  Serial.begin(115200);
  for (int pin : SENSORS) pinMode(pin, INPUT);
  digitalWrite(RELAY, !RELAY_ON);  // set off before enabling the pin, so the pump never blips on at boot
  pinMode(RELAY, OUTPUT);

  logEvent("System started.");
  loadSettings();
  startWifi();
  startServer();
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  if (restartPending && now - restartAt >= 2000) ESP.restart();

  if (now - lastLive >= LIVE_TIME) {
    lastLive = now;
    checkWifi(now);
    for (int i = 0; i < 3; i++) {
      int v = readProbe(SENSORS[i]);
      // Light smoothing between seconds; jump straight to big changes.
      live[i] = abs(v - live[i]) > 400 ? v : (live[i] * 2 + v) / 3;
    }
    Serial.println("Live: " + readings(live) + "  Pump: " + (pumpOn ? "ON" : "OFF"));

    if (historyCount == 0 || now - lastHistory >= HISTORY_TIME) {
      lastHistory = now;
      for (int i = 0; i < 3; i++) history[historyHead][i] = live[i];
      historyHead = (historyHead + 1) % HISTORY_SIZE;
      if (historyCount < HISTORY_SIZE) historyCount++;
    }
  }

  // No sensor checks while the pump runs.
  if (pumpOn) {
    if (now - pumpStart >= pumpTime) setPump(false, String(pumpTime / MINUTE) + " minutes done");
    return;
  }

  if (firstReading || checkRequested || now - lastCheck >= cfg.checkMin * MINUTE) {
    checkRequested = false;
    lastCheck = now;
    runCheck(now);
  }
}
