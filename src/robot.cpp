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

      if (xQueueReceive(normalizedSensorValuesQ, &normalized, pdMS_TO_TICKS(5)))
      {
        float kP = 1100;
        float kD = robot.kP * 0.8f; 

        float new_error = getError(normalized);
        float derivative = new_error - robot.last_error;
        float new_correction = (robot.kP * new_error) + (kD * derivative);

        xSemaphoreTake(robotMutex, portMAX_DELAY);
        robot.last_error = robot.error;
        robot.error = new_error;
        robot.correction = new_correction;
        robot.kP = kP;
        robot.kD = kD;
        //robot.base_speed = (analogRead(SPEED_POTEN) / 4095.0f) * 3000.0f;
        xSemaphoreGive(robotMutex);

        xSemaphoreTake(robotMutex, portMAX_DELAY);
        int left = robot.base_speed - (int)new_correction;
        int right = robot.base_speed + (int)new_correction;
        xSemaphoreGive(robotMutex);

        updateMotors(right, left);
      }
      break;

    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}