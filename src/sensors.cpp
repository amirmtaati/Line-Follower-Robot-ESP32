#include <Arduino.h>
#include "config.hh"
#include "types.hh"
#include "sensors.hh"

void vReadSensorValuesTask(void *parameters)
{
  while (!ready) vTaskDelay(pdMS_TO_TICKS(10));
  float temp_ir_values[N_SENSORS];

  while (true)
  {
    for (int i = 0; i < N_SENSORS; i++)
    {
      temp_ir_values[i] = analogRead(IR_SENSORS[i]);
    }

    xQueueSend(irValuesQ, &temp_ir_values, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void vNormalizeSensorValuesTask(void *parameters)
{
  while (!ready) vTaskDelay(pdMS_TO_TICKS(10));
  float temp_normalized_sensor_values[N_SENSORS];
  float ir_values[N_SENSORS];
  bool calibrated;

  while (true)
  {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    calibrated = isCalibrated;
    xSemaphoreGive(robotMutex);

    if (xQueueReceive(irValuesQ, &ir_values, portMAX_DELAY) == pdTRUE && calibrated)
    {
      xSemaphoreTake(robotMutex, portMAX_DELAY);

      float white[N_SENSORS];
      float black[N_SENSORS];
      memcpy(white, WHITE_VALUES, sizeof(white));
      memcpy(black, BLACK_VALUES, sizeof(black));

      xSemaphoreGive(robotMutex);

      for (int i = 0; i < N_SENSORS; i++)
      {
        float normalized_value = (ir_values[i] - white[i]) / (white[i] - black[i]);
        temp_normalized_sensor_values[i] = constrain(normalized_value, 0.0f, 1.0f);
      }

      xQueueSend(normalizedSensorValuesQ, &temp_normalized_sensor_values, portMAX_DELAY);
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }
}

void calibrateSensorValues()
{
  TickType_t startTime;
  long whiteSum[N_SENSORS] = {0};
  long blackSum[N_SENSORS] = {0};
  int count = 0;
  float ir_values[N_SENSORS];

  tone(BUZZER, 2000);
  startTime = xTaskGetTickCount();

  while (xTaskGetTickCount() - startTime < pdMS_TO_TICKS(2000))
  {
    if (xQueueReceive(irValuesQ, &ir_values, pdMS_TO_TICKS(10)) == pdTRUE)
    {
      for (int i = 0; i < N_SENSORS; i++)
      {
        whiteSum[i] += ir_values[i];
      }
      count++;
      vTaskDelay(pdMS_TO_TICKS(2));
    }
  }

  xSemaphoreTake(robotMutex, portMAX_DELAY);
  for (int i = 0; i < N_SENSORS; i++)
  {
    if (count == 0) continue;
    WHITE_VALUES[i] = whiteSum[i] / count;
  }
  xSemaphoreGive(robotMutex);

  noTone(BUZZER);

  vTaskDelay(pdMS_TO_TICKS(3000));

  tone(BUZZER, 1000);
  startTime = xTaskGetTickCount();
  count = 0;

  while (xTaskGetTickCount() - startTime < pdMS_TO_TICKS(2000))
  {
    if (xQueueReceive(irValuesQ, &ir_values, pdMS_TO_TICKS(10)) == pdTRUE)
    {
      for (int i = 0; i < N_SENSORS; i++)
      {
        blackSum[i] += ir_values[i];
      }
      count++;
      vTaskDelay(pdMS_TO_TICKS(2));
    }
  }

  xSemaphoreTake(robotMutex, portMAX_DELAY);
  for (int i = 0; i < N_SENSORS; i++)
  {
    if (count == 0) continue;
    BLACK_VALUES[i] = blackSum[i] / count;
  }
  xSemaphoreGive(robotMutex);

  noTone(BUZZER);
}