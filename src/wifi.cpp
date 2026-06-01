#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "credentials.hh"
#include "types.hh"
#include "wifi.hh"
#include "wifi_driver.hh"

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Line Follower</title>
  <style>
    body  { font-family: monospace; background: #111; color: #0f0; padding: 20px; }
    h1    { color: #fff; }
    .card { background: #1a1a1a; border: 1px solid #333; padding: 16px; margin: 10px 0; border-radius: 8px; }
    label { display: block; margin: 8px 0 4px; }
    input[type=number] { background: #222; color: #0f0; border: 1px solid #555; padding: 4px; width: 80px; }
    button     { background: #0a0; color: #fff; border: none; padding: 10px 20px; border-radius: 4px; cursor: pointer; margin: 4px; }
    button.red { background: #a00; }
    .value     { color: #ff0; font-weight: bold; }
    #link      { color: #888; font-size: 0.85em; }
  </style>
</head>
<body>
  <h1>Line Follower</h1>

  <div class="card">
    <div>State:      <span class="value" id="state">-</span></div>
    <div>Error:      <span class="value" id="error">-</span></div>
    <div>Correction: <span class="value" id="correction">-</span></div>
    <div>kP:         <span class="value" id="kp">-</span></div>
    <div>kD:         <span class="value" id="kd">-</span></div>
    <div>Base Speed: <span class="value" id="speed">-</span></div>
    <div>L PWM:      <span class="value" id="lpwm">-</span></div>
    <div>R PWM:      <span class="value" id="rpwm">-</span></div>
    <div>Calibrated: <span class="value" id="calibrated">-</span></div>
    <div id="link">-</div>
  </div>

  <div class="card">
    <div>Sensors (normalized):</div>
    <div>
      IR0: <span class="value" id="ir0">-</span>
      IR1: <span class="value" id="ir1">-</span>
      IR2: <span class="value" id="ir2">-</span>
      IR3: <span class="value" id="ir3">-</span>
    </div>
    <div style="margin-top:8px">White: <span class="value" id="wh">-</span></div>
    <div>Black: <span class="value" id="bl">-</span></div>
  </div>

  <div class="card">
    <label>kP:         <input type="number" id="kp_in"    value="800"  step="50"></label>
    <label>kD:         <input type="number" id="kd_in"    value="600"  step="50"></label>
    <label>Base Speed: <input type="number" id="speed_in" value="1200" step="100"></label>
    <button onclick="sendParams()">Apply</button>
  </div>

  <div class="card">
    <button onclick="cmd('/start')">START</button>
    <button class="red" onclick="cmd('/stop')">STOP</button>
    <button onclick="cmd('/calibrate')">CALIBRATE</button>
  </div>

<script>
  var inp = {
    kp:    document.getElementById('kp_in'),
    kd:    document.getElementById('kd_in'),
    speed: document.getElementById('speed_in')
  };
  var editing = false;
  var dirty   = false;

  inp.kp.onfocus = inp.kd.onfocus = inp.speed.onfocus = function(){ editing = true; };
  inp.kp.onblur  = inp.kd.onblur  = inp.speed.onblur  = function(){ editing = false; };
  inp.kp.oninput = inp.kd.oninput = inp.speed.oninput = function(){ dirty   = true; };

  function set(id, v) { document.getElementById(id).textContent = v; }

  function render(d, src) {
    set('state',      d.state);
    set('error',      (+d.error).toFixed(3));
    set('correction', (+d.correction).toFixed(1));
    set('kp',         (+d.kp).toFixed(0));
    set('kd',         (+d.kd).toFixed(0));
    set('speed',      (+d.base_speed).toFixed(0));
    set('lpwm',       d.left_pwm);
    set('rpwm',       d.right_pwm);
    set('calibrated', d.calibrated ? 'YES' : 'NO');
    set('ir0',        (+d.ir[0]).toFixed(3));
    set('ir1',        (+d.ir[1]).toFixed(3));
    set('ir2',        (+d.ir[2]).toFixed(3));
    set('ir3',        (+d.ir[3]).toFixed(3));
    set('wh', d.white.map(function(v){ return (+v).toFixed(0); }).join('  '));
    set('bl', d.black.map(function(v){ return (+v).toFixed(0); }).join('  '));
    set('link', src + ' ' + new Date().toLocaleTimeString());
    if (!editing && !dirty) {
      inp.kp.value    = (+d.kp).toFixed(0);
      inp.kd.value    = (+d.kd).toFixed(0);
      inp.speed.value = (+d.base_speed).toFixed(0);
    }
  }

  function get(path, cb) {
    var x = new XMLHttpRequest();
    x.open('GET', path, true);
    x.timeout = 2000;
    x.onload  = function(){ if (x.status === 200 && cb) cb(x.responseText); };
    x.onerror = x.ontimeout = function(){
      set('link', 'error ' + new Date().toLocaleTimeString());
    };
    x.send();
  }

  function poll() {
    get('/data', function(text){
      try { render(JSON.parse(text), 'poll'); } catch(e){}
    });
  }

  function sendParams() {
    var url = '/set?kp='  + encodeURIComponent(inp.kp.value)
            + '&kd='      + encodeURIComponent(inp.kd.value)
            + '&speed='   + encodeURIComponent(inp.speed.value);
    get(url, function(text){
      try { dirty = false; render(JSON.parse(text), 'set'); } catch(e){ poll(); }
    });
  }

  function cmd(path) {
    get(path, function(){ poll(); });
  }

  if (window.EventSource) {
    var es = new EventSource('/events');
    es.addEventListener('telemetry', function(e){
      try { render(JSON.parse(e.data), 'stream'); } catch(ex){}
    });
    es.onerror = function(){ setInterval(poll, 500); };
  } else {
    setInterval(poll, 500);
  }

  poll();
</script>
</body>
</html>
)rawliteral";

static AsyncWebServer server(80);
static AsyncEventSource events("/events");

static const char *stateToStr(State s)
{
  switch (s)
  {
  case State::STOPPED:
    return "STOPPED";
  case State::CALIBRATING:
    return "CALIBRATING";
  case State::MOVING:
    return "MOVING";
  default:
    return "UNKNOWN";
  }
}

static float safe(float v)
{
  return isfinite(v) ? v : 0.0f;
}

static void buildJson(char *buf, size_t len)
{
  xSemaphoreTake(robotMutex, portMAX_DELAY);
  RobotState r = robot;
  State s = state;
  bool cal = isCalibrated;
  float ir[N_SENSORS];
  float wh[N_SENSORS];
  float bl[N_SENSORS];
  memcpy(ir, nv, sizeof(ir));
  memcpy(wh, WHITE_VALUES, sizeof(wh));
  memcpy(bl, BLACK_VALUES, sizeof(bl));
  xSemaphoreGive(robotMutex);

  snprintf(buf, len,
           "{"
           "\"state\":\"%s\","
           "\"error\":%.4f,\"correction\":%.2f,"
           "\"kp\":%.1f,\"kd\":%.1f,\"base_speed\":%.1f,"
           "\"left_pwm\":%d,\"right_pwm\":%d,"
           "\"calibrated\":%s,"
           "\"white\":[%.1f,%.1f,%.1f,%.1f],"
           "\"black\":[%.1f,%.1f,%.1f,%.1f],"
           "\"ir\":[%.3f,%.3f,%.3f,%.3f]"
           "}",
           stateToStr(s),
           safe(r.error), safe(r.correction),
           safe(r.kP), safe(r.kD), safe(r.base_speed),
           r.left_pwm, r.right_pwm,
           cal ? "true" : "false",
           wh[0], wh[1], wh[2], wh[3],
           bl[0], bl[1], bl[2], bl[3],
           safe(ir[0]), safe(ir[1]), safe(ir[2]), safe(ir[3]));
}

static void onRoot(AsyncWebServerRequest *req)
{
  AsyncWebServerResponse *res =
      req->beginResponse_P(200, "text/html", INDEX_HTML);
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

static void onData(AsyncWebServerRequest *req)
{
  char buf[640];
  buildJson(buf, sizeof(buf));
  AsyncWebServerResponse *res =
      req->beginResponse(200, "application/json", buf);
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

static void onSet(AsyncWebServerRequest *req)
{
  xSemaphoreTake(robotMutex, portMAX_DELAY);
  if (req->hasParam("kp"))
    robot.kP = constrain(
        req->getParam("kp")->value().toFloat(), 0.0f, 5000.0f);
  if (req->hasParam("kd"))
    robot.kD = constrain(
        req->getParam("kd")->value().toFloat(), 0.0f, 5000.0f);
  if (req->hasParam("speed"))
    robot.base_speed = constrain(
        req->getParam("speed")->value().toFloat(), 0.0f, 3200.0f);
  xSemaphoreGive(robotMutex);

  char buf[640];
  buildJson(buf, sizeof(buf));
  AsyncWebServerResponse *res =
      req->beginResponse(200, "application/json", buf);
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

static void onStart(AsyncWebServerRequest *req)
{
  xSemaphoreTake(robotMutex, portMAX_DELAY);
  if (state == State::STOPPED)
    state = isCalibrated ? State::MOVING : State::CALIBRATING;
  xSemaphoreGive(robotMutex);
  req->send(200, "text/plain", "OK");
}

static void onStop(AsyncWebServerRequest *req)
{
  xSemaphoreTake(robotMutex, portMAX_DELAY);
  state = State::STOPPED;
  robot.left_pwm = 0;
  robot.right_pwm = 0;
  xSemaphoreGive(robotMutex);
  req->send(200, "text/plain", "OK");
}

static void onCalibrate(AsyncWebServerRequest *req)
{
  xSemaphoreTake(robotMutex, portMAX_DELAY);
  state = State::CALIBRATING;
  isCalibrated = false;
  robot.error = 0.0f;
  robot.last_error = 0.0f;
  robot.correction = 0.0f;
  robot.left_pwm = 0;
  robot.right_pwm = 0;
  xSemaphoreGive(robotMutex);
  req->send(200, "text/plain", "OK");
}

static void setupRoutes()
{
  server.on("/", HTTP_GET, onRoot);
  server.on("/data", HTTP_GET, onData);
  server.on("/set", HTTP_GET, onSet);
  server.on("/start", HTTP_GET, onStart);
  server.on("/stop", HTTP_GET, onStop);
  server.on("/calibrate", HTTP_GET, onCalibrate);

  events.onConnect([](AsyncEventSourceClient *client)
                   {
        char buf[640];
        buildJson(buf, sizeof(buf));
        client->send(buf, "telemetry", millis(), 1000); });
  server.addHandler(&events);
  server.begin();
}

void vWiFiTask(void *parameters)
{
  while (!ready)
    vTaskDelay(pdMS_TO_TICKS(10));

  Serial.print("Connecting to WiFi");

  WiFiResult result = wifiConnect(WIFI_SSID, WIFI_PASSWORD);

  if (result == WiFiResult::FAILED)
  {
    Serial.println("\nWiFi failed — web server disabled");
    vTaskDelete(NULL);
    return;
  }

  Serial.printf("\nConnected — open http://%s\n", wifiIPAddress());
  setupRoutes();
  Serial.println("Web server started");

  vTaskDelete(NULL);
}

void vTelemetryTask(void *parameters)
{
  while (!ready)
    vTaskDelay(pdMS_TO_TICKS(10));

  char buf[640];
  while (true)
  {
    if (events.count() > 0)
    {
      buildJson(buf, sizeof(buf));
      events.send(buf, "telemetry", millis());
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
