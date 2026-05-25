#ifndef TYPES_H
#define TYPES_H

#include "config.hh"

float IR_VALUES[N_SENSORS];
float NORMALIZED_SENSOR_VALUES[N_SENSORS];

float BLACK_VALUES[N_SENSORS];
float WHITE_VALUES[N_SENSORS];

enum class State {
    STOPPED,
    CALIBRATING,
    MOVING
};

struct RobotState {
    float error;
    float last_error;
    float correction;
    float kP;
    float base_speed;
};

extern RobotState robot;
State state = State::STOPPED;
static bool isCalibrated = false;

extern SemaphoreHandle_t robotMutex;
extern SemaphoreHandle_t btnSemaphore;

const float WEIGHTS[N_SENSORS] = {-1.0f, -0.5f, 0.5f, 1.0f};

QueueHandle_t irValuesQ;
QueueHandle_t normalizedSensorValuesQ;

SemaphoreHandle_t robotMutex;
SemaphoreHandle_t btnSemaphore;

RobotState robot = {};
volatile bool ready = false;
bool lastButtonState = HIGH;
TickType_t lastPressTime = 0;

#endif