#ifndef CONFIG_H
#define CONFIG_H

/* Motor pins for TB6612FNG driver */
extern const int PWMA = 27;
extern const int AIN1 = 25;
extern const int AIN2 = 26;

extern const int PWMB = 23;
extern const int BIN1 = 18;
extern const int BIN2 = 19;
extern const int STBY = 5;


/* TCRT5000L – KY-033 IR sensors pins */
const int N_SENSORS = 4;
const int IR0 = 32;
const int IR1 = 33;
const int IR2 = 34;
const int IR3 = 35;
extern const int IR_SENSORS[N_SENSORS] = {IR0, IR1, IR2, IR3};


/* LED pins*/
extern const int R_LED = 12;
extern const int B_LED = 13;
extern const int G_LED = 14;

extern const int BUZZER = 4;

extern const int BUTTON = 15;

TickType_t DEBOUNCE_TIME = pdMS_TO_TICKS(50);

#define BUZZER_CHANNEL 0
#define MOTOR_A_CHANNEL 1
#define MOTOR_B_CHANNEL 2

#define PWM_FREQ 5000
#define PWM_RES 12

#define BUZZER_FREQ    2000
#define BUZZER_RES     8

#endif