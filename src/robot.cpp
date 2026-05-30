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
      stopMotors();

      xSemaphoreTake(robotMutex, portMAX_DELAY);
      robot.left_pwm = 0;
      robot.right_pwm = 0;
      xSemaphoreGive(robotMutex);
      break;

    case State::CALIBRATING:
      digitalWrite(R_LED, LOW);
      digitalWrite(G_LED, LOW);
      stopMotors();

      xSemaphoreTake(robotMutex, portMAX_DELAY);
      robot.left_pwm = 0;
      robot.right_pwm = 0;
      xSemaphoreGive(robotMutex);

      calibrateSensorValues();
      xSemaphoreTake(robotMutex, portMAX_DELAY);
      isCalibrated = true;
      state = State::STOPPED;
      startAfterCalibration = false;
      robot.error = 0.0f;
      robot.last_error = 0.0f;
      robot.correction = 0.0f;
      xSemaphoreGive(robotMutex);
      break;

    case State::MOVING:
      digitalWrite(R_LED, LOW);
      digitalWrite(G_LED, HIGH);

      if (xQueueReceive(normalizedSensorValuesQ, &normalized, pdMS_TO_TICKS(5)))
      {
        float kP;
        float kD;
        float baseSpeed;
        float lastError;

        xSemaphoreTake(robotMutex, portMAX_DELAY);
        kP = robot.kP;
        kD = robot.kD;
        baseSpeed = robot.base_speed;
        lastError = robot.last_error;
        xSemaphoreGive(robotMutex);

        float new_error = getError(normalized);
        float derivative = new_error - lastError;
        float new_correction = (kP * new_error) + (kD * derivative);
        int left = constrain((int)(baseSpeed - new_correction), -3200, 3200);
        int right = constrain((int)(baseSpeed + new_correction), -3200, 3200);

        xSemaphoreTake(robotMutex, portMAX_DELAY);
        robot.last_error = new_error;
        robot.error = new_error;
        robot.correction = new_correction;
        robot.left_pwm = left;
        robot.right_pwm = right;
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
