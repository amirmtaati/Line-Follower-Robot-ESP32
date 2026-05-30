#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include "types.hh"
#include "wifi.hh"

// ── Your network credentials ──────────────────────────────
static const char* SSID     = "Our Lovely Home";
static const char* PASSWORD = "amir85THEfallenAngel76";

static AsyncWebServer server(80);
static AsyncEventSource events("/events");
static esp_netif_t* staNetif = nullptr;
static EventGroupHandle_t wifiEventGroup = nullptr;
static int wifiRetryCount = 0;

static constexpr int WIFI_CONNECTED_BIT = BIT0;
static constexpr int WIFI_FAIL_BIT = BIT1;
static constexpr int MAX_WIFI_RETRIES = 20;

static void setWiFiLed(bool connected) {
    digitalWrite(B_LED, connected ? HIGH : LOW);
}

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
    #link { color: #888; }
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
    <div>Calibrated: <span class="value" id="calibrated">-</span></div>
    <div>Link: <span id="link">-</span></div>
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
    <button onclick="sendCommand('/start')">START</button>
    <button class="red" onclick="sendCommand('/stop')">STOP</button>
    <button onclick="sendCommand('/calibrate')">CALIBRATE</button>
  </div>

<script>
  var inputs = {
    kp: document.getElementById('kp_in'),
    kd: document.getElementById('kd_in'),
    speed: document.getElementById('speed_in')
  };
  var editing = false;
  var dirtyParams = false;
  var lastEventMs = 0;
  var pollTimer = null;

  inputs.kp.onfocus = inputs.kd.onfocus = inputs.speed.onfocus = function() { editing = true; };
  inputs.kp.onblur = inputs.kd.onblur = inputs.speed.onblur = function() { editing = false; };
  inputs.kp.oninput = inputs.kd.oninput = inputs.speed.oninput = function() { dirtyParams = true; };

  function setText(id, value) {
    document.getElementById(id).textContent = value;
  }

  function syncInputs(d) {
    if (editing || dirtyParams) return;
    inputs.kp.value = d.kp.toFixed(0);
    inputs.kd.value = d.kd.toFixed(0);
    inputs.speed.value = d.base_speed.toFixed(0);
  }

  function renderData(d, source) {
    lastEventMs = Date.now();
    setText('state', d.state);
    setText('error', Number(d.error).toFixed(3));
    setText('correction', Number(d.correction).toFixed(1));
    setText('kp', Number(d.kp).toFixed(0));
    setText('kd', Number(d.kd).toFixed(0));
    setText('speed', Number(d.base_speed).toFixed(0));
    setText('lpwm', d.left_pwm);
    setText('rpwm', d.right_pwm);
    setText('calibrated', d.calibrated ? 'YES' : 'NO');
    setText('ir0', Number(d.ir[0]).toFixed(3));
    setText('ir1', Number(d.ir[1]).toFixed(3));
    setText('ir2', Number(d.ir[2]).toFixed(3));
    setText('ir3', Number(d.ir[3]).toFixed(3));
    setText('link', source + ' ' + new Date().toLocaleTimeString());
    syncInputs(d);
  }

  function requestText(path, ok) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', path, true);
    xhr.timeout = 1500;
    xhr.setRequestHeader('Cache-Control', 'no-cache');
    xhr.onload = function() {
      if (xhr.status === 200) {
        ok(xhr.responseText);
      } else {
        setText('link', 'HTTP ' + xhr.status + ' ' + new Date().toLocaleTimeString());
      }
    };
    xhr.onerror = function() {
      setText('link', 'network error ' + new Date().toLocaleTimeString());
    };
    xhr.ontimeout = function() {
      setText('link', 'timeout ' + new Date().toLocaleTimeString());
    };
    xhr.send();
  }

  function fetchData() {
    requestText('/data', function(text) {
      try {
        renderData(JSON.parse(text), 'poll');
      } catch (e) {
        setText('link', 'bad JSON: ' + text.slice(0, 32));
      }
    });
  }

  function sendParams() {
    var kp = encodeURIComponent(inputs.kp.value);
    var kd = encodeURIComponent(inputs.kd.value);
    var speed = encodeURIComponent(inputs.speed.value);
    requestText('/set?kp=' + kp + '&kd=' + kd + '&speed=' + speed, function(text) {
      try {
        dirtyParams = false;
        renderData(JSON.parse(text), 'set');
      } catch (e) {
        fetchData();
      }
    });
  }

  function sendCommand(path) {
    requestText(path, function() { fetchData(); });
  }

  function startPolling() {
    if (pollTimer) return;
    pollTimer = setInterval(fetchData, 500);
  }

  function startEvents() {
    if (!window.EventSource) {
      startPolling();
      return;
    }

    var stream = new EventSource('/events');
    stream.addEventListener('telemetry', function(event) {
      renderData(JSON.parse(event.data), 'stream');
    });
    stream.onerror = function() {
      setText('link', 'reconnecting ' + new Date().toLocaleTimeString());
      startPolling();
    };

    setInterval(function() {
      if (Date.now() - lastEventMs > 1500) startPolling();
    }, 1000);
  }

  fetchData();
  startEvents();
</script>
</body>
</html>
)rawliteral";

// ── HTTP handlers ─────────────────────────────────────────

static const char* stateToString(State s) {
    switch (s) {
        case State::STOPPED:     return "STOPPED";
        case State::CALIBRATING: return "CALIBRATING";
        case State::MOVING:      return "MOVING";
    }

    return "UNKNOWN";
}

static float jsonFloat(float value) {
    return isfinite(value) ? value : 0.0f;
}

static void buildTelemetryJson(char* json, size_t len) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    RobotState r  = robot;
    State      s  = state;
    bool calibrated = isCalibrated;
    float      ir[N_SENSORS];
    memcpy(ir, nv, sizeof(float) * N_SENSORS);
    xSemaphoreGive(robotMutex);

    snprintf(json, len,
             "{\"state\":\"%s\","
             "\"error\":%.4f,"
             "\"correction\":%.2f,"
             "\"kp\":%.1f,"
             "\"kd\":%.1f,"
             "\"base_speed\":%.1f,"
             "\"left_pwm\":%d,"
             "\"right_pwm\":%d,"
             "\"calibrated\":%s,"
             "\"white\":[%.1f,%.1f,%.1f,%.1f]," // add this
             "\"black\":[%.1f,%.1f,%.1f,%.1f]," // add this
             "\"ir\":[%.3f,%.3f,%.3f,%.3f]}",
             stateToString(s),
             jsonFloat(r.error), jsonFloat(r.correction),
             jsonFloat(r.kP), jsonFloat(r.kD),
             jsonFloat(r.base_speed),
             r.left_pwm, r.right_pwm,
             calibrated ? "true" : "false",
             WHITE_VALUES[0], WHITE_VALUES[1], WHITE_VALUES[2], WHITE_VALUES[3],
             BLACK_VALUES[0], BLACK_VALUES[1], BLACK_VALUES[2], BLACK_VALUES[3],
             jsonFloat(ir[0]), jsonFloat(ir[1]), jsonFloat(ir[2]), jsonFloat(ir[3]));
}

static void handleData(AsyncWebServerRequest* request) {
    char json[512];
    buildTelemetryJson(json, sizeof(json));

    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response->addHeader("Pragma", "no-cache");
    request->send(response);
}

static void handlePing(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "pong");
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}

static void handleSet(AsyncWebServerRequest* request) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);

    if (request->hasParam("kp"))
        robot.kP = constrain(request->getParam("kp")->value().toFloat(), 0.0f, 5000.0f);
    if (request->hasParam("kd"))
        robot.kD = constrain(request->getParam("kd")->value().toFloat(), 0.0f, 5000.0f);
    if (request->hasParam("speed"))
        robot.base_speed = constrain(request->getParam("speed")->value().toFloat(), 0.0f, 3200.0f);

    xSemaphoreGive(robotMutex);

    char json[512];
    buildTelemetryJson(json, sizeof(json));
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response->addHeader("Pragma", "no-cache");
    request->send(response);
}

static void handleStart(AsyncWebServerRequest* request) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    if (state == State::STOPPED) {
        if (isCalibrated) {
            state = State::MOVING;
        } else {
            startAfterCalibration = false;
            state = State::CALIBRATING;
        }
    }
    xSemaphoreGive(robotMutex);
    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}

static void handleStop(AsyncWebServerRequest* request) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    state = State::STOPPED;
    startAfterCalibration = false;
    robot.left_pwm = 0;
    robot.right_pwm = 0;
    xSemaphoreGive(robotMutex);
    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}

static void handleCalibrate(AsyncWebServerRequest* request) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    state = State::CALIBRATING;
    isCalibrated = false;
    startAfterCalibration = false;
    robot.error = 0.0f;
    robot.last_error = 0.0f;
    robot.correction = 0.0f;
    robot.left_pwm = 0;
    robot.right_pwm = 0;
    xSemaphoreGive(robotMutex);
    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}

// ── WiFi task ─────────────────────────────────────────────

static void setupRoutes() {
    static bool routesConfigured = false;
    if (routesConfigured) return;

    server.on("/",          HTTP_GET, [](AsyncWebServerRequest* r){
        AsyncWebServerResponse* response = r->beginResponse_P(200, "text/html", INDEX_HTML);
        response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        response->addHeader("Pragma", "no-cache");
        r->send(response);
    });
    server.on("/data",      HTTP_GET, handleData);
    server.on("/ping",      HTTP_GET, handlePing);
    server.on("/set",       HTTP_GET, handleSet);
    server.on("/start",     HTTP_GET, handleStart);
    server.on("/stop",      HTTP_GET, handleStop);
    server.on("/calibrate", HTTP_GET, handleCalibrate);
    events.onConnect([](AsyncEventSourceClient* client) {
        char json[512];
        buildTelemetryJson(json, sizeof(json));
        client->send(json, "telemetry", millis(), 1000);
    });
    server.addHandler(&events);

    routesConfigured = true;
}

static bool checkEsp(esp_err_t err, const char* action) {
    if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) return true;

    Serial.printf("%s failed: %s (%d)\n", action, esp_err_to_name(err), err);
    return false;
}

static void wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
        setWiFiLed(false);
        if (wifiRetryCount < MAX_WIFI_RETRIES) {
            wifiRetryCount++;
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
        }
        return;
    }

    if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        wifiRetryCount = 0;
        setWiFiLed(true);
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
    }
}

bool startWiFiServer() {
    static bool wifiStarted = false;
    if (wifiStarted) return true;

    setWiFiLed(false);

    wifiEventGroup = xEventGroupCreate();
    if (wifiEventGroup == nullptr) {
        Serial.println("WiFi event group allocation failed - web server disabled");
        return false;
    }

    if (!checkEsp(esp_netif_init(), "esp_netif_init")) {
        return false;
    }

    if (!checkEsp(esp_event_loop_create_default(), "esp_event_loop_create_default")) {
        return false;
    }

    staNetif = esp_netif_create_default_wifi_sta();
    if (staNetif == nullptr) {
        Serial.println("esp_netif_create_default_wifi_sta failed - web server disabled");
        return false;
    }

    esp_netif_set_hostname(staNetif, "line-follower");

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    config.nvs_enable = false;

    if (!checkEsp(esp_wifi_init(&config), "esp_wifi_init")) {
        return false;
    }

    if (!checkEsp(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEventHandler, nullptr, nullptr),
                  "register WIFI_EVENT handler")) {
        return false;
    }

    if (!checkEsp(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifiEventHandler, nullptr, nullptr),
                  "register IP_EVENT handler")) {
        return false;
    }

    if (!checkEsp(esp_wifi_set_storage(WIFI_STORAGE_RAM), "esp_wifi_set_storage")) {
        return false;
    }

    if (!checkEsp(esp_wifi_set_mode(WIFI_MODE_STA), "esp_wifi_set_mode")) {
        return false;
    }

    wifi_config_t wifiConfig = {};
    strlcpy(reinterpret_cast<char*>(wifiConfig.sta.ssid), SSID, sizeof(wifiConfig.sta.ssid));
    strlcpy(reinterpret_cast<char*>(wifiConfig.sta.password), PASSWORD, sizeof(wifiConfig.sta.password));
    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    if (!checkEsp(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig), "esp_wifi_set_config")) {
        return false;
    }

    if (!checkEsp(esp_wifi_set_ps(WIFI_PS_NONE), "esp_wifi_set_ps")) {
        return false;
    }

    wifiRetryCount = 0;
    xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    Serial.print("Connecting to WiFi");

    if (!checkEsp(esp_wifi_start(), "esp_wifi_start")) {
        return false;
    }

    EventBits_t bits = 0;
    while ((bits & (WIFI_CONNECTED_BIT | WIFI_FAIL_BIT)) == 0) {
        bits = xEventGroupWaitBits(
            wifiEventGroup,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(500)
        );
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }

    if (bits & WIFI_FAIL_BIT) {
        Serial.println("\nWiFi failed - running without web server");
        setWiFiLed(false);
        esp_wifi_stop();
        return false;
    }

    esp_netif_ip_info_t ipInfo;
    if (!checkEsp(esp_netif_get_ip_info(staNetif, &ipInfo), "esp_netif_get_ip_info")) {
        return false;
    }

    IPAddress ip(ipInfo.ip.addr);
    Serial.printf("\nConnected — open http://%s\n", ip.toString().c_str());

    setupRoutes();
    server.begin();
    Serial.println("Web server started");
    setWiFiLed(true);
    wifiStarted = true;
    return true;
}

void vWiFiTask(void* parameters) {
    while (!ready) vTaskDelay(pdMS_TO_TICKS(10));

    startWiFiServer();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vTelemetryTask(void* parameters) {
    while (!ready) vTaskDelay(pdMS_TO_TICKS(10));

    char json[512];
    while (true) {
        if (events.count() > 0) {
            buildTelemetryJson(json, sizeof(json));
            events.send(json, "telemetry", millis());
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
