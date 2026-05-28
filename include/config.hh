#ifndef CONFIG_HH
#define CONFIG_HH

#include <Arduino.h>

// Motor pins
extern const int PWMA;
extern const int AIN1;
extern const int AIN2;
extern const int PWMB;
extern const int BIN1;
extern const int BIN2;
extern const int STBY;

// IR sensors
extern const int N_SENSORS;
extern const int IR_SENSORS[];

// LEDs
extern const int R_LED;
extern const int G_LED;
extern const int B_LED;

// Other
extern const int BUZZER;
extern const int BUTTON;
extern const int POTEN;
extern const int SPEED_POTEN;

// LEDC channels — these are fine as #define, they're not variables
#define BUZZER_CHANNEL  0
#define MOTOR_A_CHANNEL 2
#define MOTOR_B_CHANNEL 4
#define PWM_FREQ        5000
#define PWM_RES         12
#define BUZZER_FREQ     2000
#define BUZZER_RES      8

// FreeRTOS
extern TickType_t DEBOUNCE_TIME;

#endif