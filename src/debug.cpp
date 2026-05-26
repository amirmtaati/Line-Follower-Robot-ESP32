#include <Arduino.h>
#include "types.hh"
#include "debug.hh"

void printControlParameters() {
  Serial.print("error: ");
  Serial.println(robot.error);

  Serial.print("kP: ");
  Serial.println(robot.kP);
}

void printSensorValues() {
  float values[N_SENSORS];
  if (xQueueReceive(irValuesQ, &values, pdMS_TO_TICKS(10))) {
    for (int i = 0; i < N_SENSORS; i++) {
      Serial.print("IR");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(values[i]);
      Serial.print(", ");
    }
    Serial.println();
  }
}

void printNormalizedSensorValues() {
    for (int i = 0; i < N_SENSORS; i++) {
      Serial.print("IR");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(nv[i]);
      Serial.print(", ");
    }
    Serial.println();
}

void printCalibrationValues() {
  Serial.print("White values: ");
  for (int i = 0; i < N_SENSORS; i++) {
    Serial.print(WHITE_VALUES[i]);
    Serial.print(", ");
  }
  Serial.println();

  Serial.print("Black values: ");
  for (int i = 0; i < N_SENSORS; i++) {
    Serial.print(BLACK_VALUES[i]);
    Serial.print(", ");
  }
  Serial.println();
}

void vDebugTask(void* parameters) {
  while (true) {
    //printCalibrationValues();
    printNormalizedSensorValues();
    //printSensorValues();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}