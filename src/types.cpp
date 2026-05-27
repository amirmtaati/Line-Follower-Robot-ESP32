// types.cpp — definitions, one place
#include "types.hh"

float BLACK_VALUES[4]               = {0};
float WHITE_VALUES[4]               = {0};
const float WEIGHTS[4]              = {-1.0f, -0.5f, 0.5f, 1.0f};
float nv[4] = {0};

RobotState robot                    = {};
State state                         = State::STOPPED;
bool isCalibrated                   = false;
bool ready                          = false;
bool lastButtonState                = HIGH;
TickType_t lastPressTime            = 0;

SemaphoreHandle_t robotMutex        = nullptr;
SemaphoreHandle_t btnSemaphore      = nullptr;
QueueHandle_t irValuesQ             = nullptr;
QueueHandle_t normalizedSensorValuesQ = nullptr;

