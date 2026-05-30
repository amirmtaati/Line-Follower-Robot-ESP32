#include <Arduino.h>
#include <WiFi.h>
#include "config.hh"
#include "types.hh"
#include "motor.hh"
#include "sensors.hh"
#include "utils.hh"
#include "robot.hh"
#include "debug.hh"
#include "wifi.hh"

void setup()
{
  Serial.begin(115200);

  WiFi.begin("Our Lovely Home", "amir85THEfallenAngel76");
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  pinMode(R_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);
  pinMode(B_LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  for (int i = 0; i < N_SENSORS; i++)
  {
    pinMode(IR_SENSORS[i], INPUT);
  }

  robotMutex = xSemaphoreCreateMutex();
  btnSemaphore = xSemaphoreCreateBinary();

  irValuesQ = xQueueCreate(1, sizeof(float) * N_SENSORS);
  normalizedSensorValuesQ = xQueueCreate(1, sizeof(float) * N_SENSORS);

  xTaskCreate(vReadSensorValuesTask, "Read IR sensors' values", 4096, NULL, 3, NULL);
  xTaskCreate(vNormalizeSensorValuesTask, "Normalize IR sensors' values", 4096, NULL, 3, NULL);
  xTaskCreate(vButtonTask, "Check for button being pressed", 2048, NULL, 2, NULL);
  xTaskCreate(vRobotTask, "Main robot task", 8192, NULL, 2, NULL);
  // xTaskCreate(vDebugTask, "debug bro", 4096, NULL, 1, NULL);
  xTaskCreate(vWiFiTask, "WiFi server", 8192, NULL, 1, NULL);

  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RES);
  ledcAttachPin(BUZZER, BUZZER_CHANNEL);
  setupMotors();

  ready = true;
}

// void setup() {
//   WiFi.mode(WIFI_STA);
//    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
// WiFi.setHostname("esp32");
//   WiFi.begin("Our Lovely Home", "amir85THEfallenAngel76");
//   Serial.print("Connecting...");
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print('.');
//     delay(1000);
//   }
//   Serial.println(WiFi.localIP());

// }

void loop()
{
}
