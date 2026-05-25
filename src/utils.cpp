#include <Arduino.h>
#include "types.hh"
#include "config.hh"
#include "utils.hh"

float getError(float* normalized) {
  float numerator   = 0;
  float denominator = 0;
  for (int i = 0; i < N_SENSORS; i++) {
    numerator   += WEIGHTS[i] * normalized[i];
    denominator += normalized[i];
  }
  if (denominator == 0) {
    return robot.last_error;
  }

  return numerator / denominator;
}

void vButtonTask(void *parameters)
{
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
        if (calibrated)
        {
          xSemaphoreTake(robotMutex, portMAX_DELAY);
          state = State::MOVING;
          xSemaphoreGive(robotMutex);
        }
        break;

      case State::MOVING:
        xSemaphoreTake(robotMutex, portMAX_DELAY);
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