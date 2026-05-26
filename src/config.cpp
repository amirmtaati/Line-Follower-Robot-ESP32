#include "config.hh"

// Motor pins
const int PWMA = 27;
const int AIN1 = 25;
const int AIN2 = 26;
const int PWMB = 23;
const int BIN1 = 18;
const int BIN2 = 19;
const int STBY = 5;

// IR sensors
const int N_SENSORS = 4;
const int IR_SENSORS[] = {32, 33, 34, 35};

// LEDs
const int R_LED = 12;
const int G_LED = 14;
const int B_LED = 13;

// Other
const int BUZZER = 4;
const int BUTTON = 15;

TickType_t DEBOUNCE_TIME = pdMS_TO_TICKS(50);