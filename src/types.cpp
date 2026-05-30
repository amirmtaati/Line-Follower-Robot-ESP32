// types.cpp — definitions, one place
#include "types.hh"

float BLACK_VALUES[4]               = {0};
float WHITE_VALUES[4]               = {0};
const float WEIGHTS[4] = {-3.0f, -1.0f, 1.0f, 3.0f};
float nv[4] = {0};

RobotState robot                    = {0.0f, 0.0f, 0.0f, 800.0f, 600.0f, 1200.0f, 0, 0};
State state                         = State::STOPPED;
bool isCalibrated                   = false;
bool startAfterCalibration          = false;
bool ready                          = false;
bool lastButtonState                = HIGH;
TickType_t lastPressTime            = 0;

SemaphoreHandle_t robotMutex        = nullptr;
SemaphoreHandle_t btnSemaphore      = nullptr;
QueueHandle_t irValuesQ             = nullptr;
QueueHandle_t normalizedSensorValuesQ = nullptr;
