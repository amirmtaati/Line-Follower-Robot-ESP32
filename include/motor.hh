#ifndef MOTOR_H
#define MOTOR_H

void setupMotors();
void stopMotors();
static void writeMotor(int channel, int inp1, int inp2, int speed);
void updateMotors(int rightSpeed, int leftSpeed);

#endif