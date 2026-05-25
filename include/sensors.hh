#ifndef SENSORS_H
#define SENSORS_H


void vReadSensorValuesTask(void *parameters);
void vNormalizeSensorValuesTask(void *parameters);
void calibrateSensorValues();

#endif