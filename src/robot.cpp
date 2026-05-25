#include <Arduino.h>
#include "config.hh"
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

      updateMotors(2000 * 4, 1.2000);

      if (xQueueReceive(normalizedSensorValuesQ, &normalized, pdMS_TO_TICKS(10))) {
        robot.last_error = robot.error;
        robot.error = getError(normalized);
        robot.kP = 1000;
        robot.correction = robot.kP * robot.error;
        robot.base_speed = 2000;
        
        int left = robot.base_speed + (int)robot.correction;
        int right = robot.base_speed - (int)robot.correction;

        if (robot.error == 0.0) {
          digitalWrite(B_LED, HIGH);
        } else {
          digitalWrite(R_LED, LOW);
        }

       // updateMotors(left, right);
      }
      break;

    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}