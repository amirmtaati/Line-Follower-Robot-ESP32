#include <Arduino.h>
#include "types.hh"
#include "motor.hh"
#include "sensors.hh"
#include "utils.hh"
#include "robot.hh"

void vRobotTask(void *parameters)
{
  while (!ready) vTaskDelay(pdMS_TO_TICKS(10));
  float normalized[N_SENSORS];
  State localState;

  while (true)
  {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    localState = state;
    xSemaphoreGive(robotMutex);

    switch (localState)
    {
    case State::STOPPED:
      digitalWrite(R_LED, HIGH);
      digitalWrite(G_LED, LOW);
      digitalWrite(B_LED, LOW);
      stopMotors();

      break;

    case State::CALIBRATING:
      digitalWrite(R_LED, LOW);
      digitalWrite(G_LED, LOW);
      digitalWrite(B_LED, HIGH);
      calibrateSensorValues();
      xSemaphoreTake(robotMutex, portMAX_DELAY);
      state = State::STOPPED;
      isCalibrated = true;
      xSemaphoreGive(robotMutex);
      break;

    case State::MOVING:
      digitalWrite(R_LED, LOW);
      digitalWrite(G_LED, HIGH);
      digitalWrite(B_LED, LOW);

      if (xQueueReceive(normalizedSensorValuesQ, &normalized, pdMS_TO_TICKS(10)))
      {
        robot.kP = 1000;
        float new_error = getError(normalized);
        float new_correction = robot.kP * new_error;

        xSemaphoreTake(robotMutex, portMAX_DELAY);
        robot.last_error = robot.error;
        robot.error = new_error;
        robot.correction = new_correction;
        robot.base_speed = 2000;
        xSemaphoreGive(robotMutex);

        int left = robot.base_speed + (int)new_correction;
        int right = robot.base_speed - (int)new_correction;
        updateMotors(right, left);
      }
      break;

    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}