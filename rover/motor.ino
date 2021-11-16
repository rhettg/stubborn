extern "C" {
    #include "evt.h"
    #include "events.h"
    #include "errors.h"
}

uint8_t enablePin[] = {9, 10};
#ifdef ENABLE_SERIAL
uint8_t motorPin1[] = {8, 6};
#else
uint8_t motorPin1[] = {1, 6};
#endif
uint8_t motorPin2[] = {7, 5};

struct {
  EVT_Event_t event = {EVT_TYPE_MOTOR_SPEED};
  int Aval = 0;
  int Bval = 0;
} motorSpeedEvent;

void motorInit()
{
  for (int motor = 0; motor < 2; motor++) {
    pinMode(enablePin[motor], OUTPUT);
    pinMode(motorPin1[motor], OUTPUT);
    pinMode(motorPin2[motor], OUTPUT);
  }

  setMotor(0, 0);
  setMotor(1, 0);
}

void changeSpeed(int A, int B)
{
  motorSpeedEvent.Aval = A;
  motorSpeedEvent.Bval = B;

  if (0 != EVT_notify(&evt, (EVT_Event_t *)&motorSpeedEvent)) {
    Error(ERR_EVT);
  }
}

void setMotor(int motor, int speed) {
  int out = 0;
  uint16_t bias = 0;

  if (0 != TBL_get(&tbl, TBL_VAL_MOTOR_BIAS_A+motor, &bias)) {
    Error(ERR_TBL_GET);
  }
  
  if (speed > 0) {
    digitalWrite(motorPin1[motor], HIGH);
    digitalWrite(motorPin2[motor], LOW);

    out = speed;
  } else {
    digitalWrite(motorPin1[motor], LOW);
    digitalWrite(motorPin2[motor], HIGH);

    out = -speed;
  }

  if (out < bias) {
    speed = 0;
    bias = 0;
  }
  
  analogWrite(enablePin[motor], out - bias);
}

void handleMotorSpeed(EVT_Event_t *e)
{
  if (EVT_TYPE_MOTOR_SPEED != e->type) {
    return;
  }

  if (e != (EVT_Event_t *)&motorSpeedEvent) {
    return;
  }

  setMotor(0, motorSpeedEvent.Aval);
  setMotor(1, motorSpeedEvent.Bval);

  if (0 != TO_set(&to, TO_PARAM_MOTOR_A, motorSpeedEvent.Aval)) {
    Error(ERR_TO_SET);
  }
  if (0 != TO_set(&to, TO_PARAM_MOTOR_B, motorSpeedEvent.Bval)) {
    Error(ERR_TO_SET);
  }
}