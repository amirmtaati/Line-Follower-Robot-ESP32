#include <Arduino.h>
#include "config.hh"
#include "motor.hh"

void setupMotors()
{
  digitalWrite(STBY, HIGH);

  ledcSetup(MOTOR_A_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWMA, MOTOR_A_CHANNEL);

  ledcSetup(MOTOR_B_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWMB, MOTOR_B_CHANNEL);
}

void stopMotors()
{
  digitalWrite(STBY, LOW);
}

static void writeMotor(int ch, int in1, int in2, int speed)
{
  speed = constrain(speed, -4095, 4095);

  if (speed > 0)
  {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    ledcWrite(ch, speed);
  }
  else if (speed < 0)
  {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    ledcWrite(ch, -speed);
  }
  else
  {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, HIGH);
    ledcWrite(ch, 0);
  }
}

void updateMotors(int rightSpeed, int leftSpeed)
{
  digitalWrite(STBY, HIGH);

  leftSpeed = constrain(leftSpeed, -3200, 3200);
  rightSpeed = constrain(rightSpeed, -3200, 3200);

  writeMotor(MOTOR_A_CHANNEL, AIN1, AIN2, rightSpeed);
  writeMotor(MOTOR_B_CHANNEL, BIN1, BIN2, leftSpeed);
}