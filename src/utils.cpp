#include <Arduino.h>
#include "types.hh"
#include "utils.hh"

float getError(float* normalized) {
  float numerator   = 0;
  float denominator = 0;
  for (int i = 0; i < N_SENSORS; i++) {
    if (isnan(normalized[i]) || normalized[i] < 0.0f || normalized[i] > 1.0f) {
        continue;
    }

    numerator   += WEIGHTS[i] * normalized[i];
    denominator += normalized[i];
  }
  if (denominator == 0) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    float lastError = robot.last_error;
    xSemaphoreGive(robotMutex);
    return isfinite(lastError) ? lastError : 0.0f;
  }

  return numerator / denominator;
}

float getKp() {
  return (analogRead(POTEN) / 4095.0f) * 2000.0f;
}

void vButtonTask(void *parameters)
{
  while (!ready) vTaskDelay(pdMS_TO_TICKS(10));

  while (true)
  {
    bool currentState = digitalRead(BUTTON);

    if (currentState == LOW &&
        lastButtonState == HIGH &&
        (xTaskGetTickCount() - lastPressTime > DEBOUNCE_TIME))
    {
      lastPressTime = xTaskGetTickCount();

      xSemaphoreTake(robotMutex, portMAX_DELAY);
      State localState = state;
      bool calibrated = isCalibrated;
      xSemaphoreGive(robotMutex);

      switch (localState)
      {
      case State::STOPPED:
        if (calibrated == false)
        {
          xSemaphoreTake(robotMutex, portMAX_DELAY);
          startAfterCalibration = false;
          state = State::CALIBRATING;
          xSemaphoreGive(robotMutex);
        }
        else
        {

          xSemaphoreTake(robotMutex, portMAX_DELAY);
          state = State::MOVING;
          xSemaphoreGive(robotMutex);
        }
        break;

      case State::CALIBRATING:
        break;

      case State::MOVING:
        xSemaphoreTake(robotMutex, portMAX_DELAY);
        startAfterCalibration = false;
        state = State::STOPPED;
        xSemaphoreGive(robotMutex);
        break;

      default:
        break;
      }
    }

    lastButtonState = currentState;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
