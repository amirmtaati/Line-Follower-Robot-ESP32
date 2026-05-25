#include <Arduino.h>
#include "config.hh"
#include "types.hh"
#include "debug.hh"

void printControlParameters() {
  Serial.print("error: ");
  Serial.println(robot.error);

  Serial.print("kP: ");
  Serial.println(robot.kP);
}

void printSensorValues() {
  Serial.print("IR0: ");
  Serial.print(IR_VALUES[0]);

  Serial.print(", IR1: ");
  Serial.print(IR_VALUES[1]);

  Serial.print(", IR2: ");
  Serial.print(IR_VALUES[2]);

  Serial.print(", IR3: ");
  Serial.print(IR_VALUES[3]);
}

void printNormalizedSensorValues() {
  float values[N_SENSORS];
  if (xQueueReceive(normalizedSensorValuesQ, &values, pdMS_TO_TICKS(10))) {
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
    printCalibrationValues();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}