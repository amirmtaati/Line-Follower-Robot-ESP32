// types.hh — declarations only
#ifndef TYPES_HH
#define TYPES_HH

#include "config.hh"

enum class State { STOPPED, CALIBRATING, MOVING };

struct RobotState {
  float error;
  float last_error;
  float correction;
  float kP;
  float base_speed;
};

// Declare everything extern — defined in types.cpp
extern float BLACK_VALUES[];
extern float WHITE_VALUES[];
extern const float WEIGHTS[];

extern RobotState robot;
extern State state;
extern bool isCalibrated;
extern bool ready;
extern bool lastButtonState;
extern TickType_t lastPressTime;

extern SemaphoreHandle_t robotMutex;
extern SemaphoreHandle_t btnSemaphore;
extern QueueHandle_t irValuesQ;
extern QueueHandle_t normalizedSensorValuesQ;
extern float nv[];

#endif