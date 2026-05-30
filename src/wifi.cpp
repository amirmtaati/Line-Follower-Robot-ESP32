#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "types.hh"
#include "wifi.hh"

// ── Your network credentials ──────────────────────────────
static const char* SSID     = "Our Lovely Home";
static const char* PASSWORD = "amir85THEfallenAngel76";

static AsyncWebServer server(80);

// ── HTML page — served once when browser connects ─────────
// Embedded directly in the binary, no SD card needed
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Line Follower</title>
  <style>
    body { font-family: monospace; background: #111; color: #0f0; padding: 20px; }
    h1   { color: #fff; }
    .card { background: #1a1a1a; border: 1px solid #333; padding: 16px; margin: 10px 0; border-radius: 8px; }
    label { display: block; margin: 8px 0 4px; }
    input[type=range] { width: 100%; }
    input[type=number] { background: #222; color: #0f0; border: 1px solid #555; padding: 4px; width: 80px; }
    button { background: #0a0; color: #fff; border: none; padding: 10px 20px; border-radius: 4px; cursor: pointer; margin: 4px; }
    button.red { background: #a00; }
    .value { color: #ff0; font-weight: bold; }
    #state { font-size: 1.4em; }
  </style>
</head>
<body>
  <h1>Line Follower Monitor</h1>

  <!-- Live state -->
  <div class="card">
    <div>State: <span class="value" id="state">-</span></div>
    <div>Error: <span class="value" id="error">-</span></div>
    <div>Correction: <span class="value" id="correction">-</span></div>
    <div>kP: <span class="value" id="kp">-</span></div>
    <div>kD: <span class="value" id="kd">-</span></div>
    <div>Base Speed: <span class="value" id="speed">-</span></div>
    <div>L PWM: <span class="value" id="lpwm">-</span></div>
    <div>R PWM: <span class="value" id="rpwm">-</span></div>
  </div>

  <!-- Sensor values -->
  <div class="card">
    <div>Sensors (normalized):</div>
    <div>IR0: <span class="value" id="ir0">-</span>
         IR1: <span class="value" id="ir1">-</span>
         IR2: <span class="value" id="ir2">-</span>
         IR3: <span class="value" id="ir3">-</span></div>
  </div>

  <!-- Controls -->
  <div class="card">
    <label>kP: <input type="number" id="kp_in" value="800" step="50"></label>
    <label>kD: <input type="number" id="kd_in" value="600" step="50"></label>
    <label>Base Speed: <input type="number" id="speed_in" value="1200" step="100"></label>
    <button onclick="sendParams()">Apply</button>
  </div>

  <!-- Robot control -->
  <div class="card">
    <button onclick="fetch('/start')">START</button>
    <button class="red" onclick="fetch('/stop')">STOP</button>
    <button onclick="fetch('/calibrate')">CALIBRATE</button>
  </div>

<script>
  // Poll live data every 200ms
  function updateData() {
    fetch('/data')
      .then(r => r.json())
      .then(d => {
        document.getElementById('state').textContent      = d.state;
        document.getElementById('error').textContent      = d.error.toFixed(3);
        document.getElementById('correction').textContent = d.correction.toFixed(1);
        document.getElementById('kp').textContent         = d.kp.toFixed(0);
        document.getElementById('kd').textContent         = d.kd.toFixed(0);
        document.getElementById('speed').textContent      = d.base_speed.toFixed(0);
        document.getElementById('lpwm').textContent       = d.left_pwm;
        document.getElementById('rpwm').textContent       = d.right_pwm;
        document.getElementById('ir0').textContent        = d.ir[0].toFixed(3);
        document.getElementById('ir1').textContent        = d.ir[1].toFixed(3);
        document.getElementById('ir2').textContent        = d.ir[2].toFixed(3);
        document.getElementById('ir3').textContent        = d.ir[3].toFixed(3);
      })
      .catch(() => {});
  }

  function sendParams() {
    const kp    = document.getElementById('kp_in').value;
    const kd    = document.getElementById('kd_in').value;
    const speed = document.getElementById('speed_in').value;
    fetch(`/set?kp=${kp}&kd=${kd}&speed=${speed}`);
  }

  setInterval(updateData, 200);
  updateData();
</script>
</body>
</html>
)rawliteral";

// ── HTTP handlers ─────────────────────────────────────────

static void handleData(AsyncWebServerRequest* request) {
    // Snapshot all state under mutex — same pattern as debug task
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    RobotState r  = robot;
    State      s  = state;
    float      ir[N_SENSORS];
    memcpy(ir, nv, sizeof(float) * N_SENSORS);
    int lpwm = (int)(r.base_speed - r.correction);
    int rpwm = (int)(r.base_speed + r.correction);
    xSemaphoreGive(robotMutex);

    const char* stateStr = "UNKNOWN";
    switch (s) {
        case State::STOPPED:     stateStr = "STOPPED";     break;
        case State::CALIBRATING: stateStr = "CALIBRATING"; break;
        case State::MOVING:      stateStr = "MOVING";      break;
    }

    // Build JSON response
    char json[512];
    snprintf(json, sizeof(json),
        "{\"state\":\"%s\","
        "\"error\":%.4f,"
        "\"correction\":%.2f,"
        "\"kp\":%.1f,"
        "\"kd\":%.1f,"
        "\"base_speed\":%.1f,"
        "\"left_pwm\":%d,"
        "\"right_pwm\":%d,"
        "\"ir\":[%.3f,%.3f,%.3f,%.3f]}",
        stateStr,
        r.error, r.correction,
        r.kP, r.kD,
        r.base_speed,
        lpwm, rpwm,
        ir[0], ir[1], ir[2], ir[3]
    );

    request->send(200, "application/json", json);
}

static void handleSet(AsyncWebServerRequest* request) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);

    if (request->hasParam("kp"))
        robot.kP = request->getParam("kp")->value().toFloat();
    if (request->hasParam("kd"))
        robot.kD = request->getParam("kd")->value().toFloat();
    if (request->hasParam("speed"))
        robot.base_speed = request->getParam("speed")->value().toFloat();

    xSemaphoreGive(robotMutex);
    request->send(200, "text/plain", "OK");
}

static void handleStart(AsyncWebServerRequest* request) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    if (state == State::STOPPED)
        state = isCalibrated ? State::MOVING : State::CALIBRATING;
    xSemaphoreGive(robotMutex);
    request->send(200, "text/plain", "OK");
}

static void handleStop(AsyncWebServerRequest* request) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    state = State::STOPPED;
    xSemaphoreGive(robotMutex);
    request->send(200, "text/plain", "OK");
}

static void handleCalibrate(AsyncWebServerRequest* request) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    state = State::CALIBRATING;
    xSemaphoreGive(robotMutex);
    request->send(200, "text/plain", "OK");
}

// ── WiFi task ─────────────────────────────────────────────

void vWiFiTask(void* parameters) {
    while (!ready) vTaskDelay(pdMS_TO_TICKS(10));
    vTaskDelay(pdMS_TO_TICKS(2000));

    WiFi.mode(WIFI_STA);
    vTaskDelay(pdMS_TO_TICKS(100));  // let mode set before begin
    WiFi.begin(SSID, PASSWORD);

    Serial.print("Connecting to WiFi");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi failed — running without web server");
        vTaskDelete(NULL);
        return;
    }

    Serial.printf("\nConnected — open http://%s\n",
        WiFi.localIP().toString().c_str());

    server.on("/",          HTTP_GET, [](AsyncWebServerRequest* r){
        r->send_P(200, "text/html", INDEX_HTML);
    });
    server.on("/data",      HTTP_GET, handleData);
    server.on("/set",       HTTP_GET, handleSet);
    server.on("/start",     HTTP_GET, handleStart);
    server.on("/stop",      HTTP_GET, handleStop);
    server.on("/calibrate", HTTP_GET, handleCalibrate);

    server.begin();
    Serial.println("Web server started");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
