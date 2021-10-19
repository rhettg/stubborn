extern "C" {
    #include "ci.h"
    #include "to.h"
    #include "tmr.h"
    #include "events.h"
    #include "errors.h"
}

int handleCmdNoOp(uint8_t data[CI_MAX_DATA])
{
  return CI_R_OK;
}

int handleCmdBoom(uint8_t data[CI_MAX_DATA])
{
  Error(ERR_CMD);
  return CI_R_ERR_FAILED;
}

int handleCmdClear(uint8_t data[CI_MAX_DATA])
{
  if (0 != TO_set(&to, TO_PARAM_ERROR, 0)) {
    Error(ERR_TO_SET);
    return CI_R_ERR_FAILED;
  } else {
    return CI_R_OK;
  }
}

int handleCmdStop(uint8_t data[CI_MAX_DATA])
{
    changeSpeed(0, 0);
    return CI_R_OK;
}

int scaleCmdDuration(uint8_t v)
{
    if (v == 0) {
      return 1000;
    } else {
      return 100 * v;
    }
}

#define MIN_MOTOR_SPEED 150
#define MAX_MOTOR_SPEED 250
#define QUARTER_TURN_DURATION 4

int handleCmdFwd(uint8_t data[CI_MAX_DATA])
{
    uint8_t t1 = data[0];

    changeSpeed(MIN_MOTOR_SPEED, MIN_MOTOR_SPEED);

    if (0 != TMR_enqueue(&tmr, &ciStopEvent, millis()+scaleCmdDuration(t1))) {
      Error(ERR_TMR_ENQUEUE);
    }

    return CI_R_OK;
}

int handleCmdFfwd(uint8_t data[CI_MAX_DATA])
{
    uint8_t t1 = data[0];

    changeSpeed(MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);

    if (0 != TMR_enqueue(&tmr, &ciStopEvent, millis()+scaleCmdDuration(t1))) {
      Error(ERR_TMR_ENQUEUE);
    }

    return CI_R_OK;
}

int handleCmdBck(uint8_t data[CI_MAX_DATA])
{
    uint8_t t1 = data[0];

    changeSpeed(-MIN_MOTOR_SPEED, -MIN_MOTOR_SPEED);

    if (0 != TMR_enqueue(&tmr, &ciStopEvent, millis()+scaleCmdDuration(t1))) {
      Error(ERR_TMR_ENQUEUE);
    }

    return CI_R_OK;
}

int handleCmdRT(uint8_t data[CI_MAX_DATA])
{
    uint8_t t1 = data[0];

    changeSpeed(MIN_MOTOR_SPEED, -MIN_MOTOR_SPEED);

    if (0 == t1) {
      t1 = QUARTER_TURN_DURATION;
    }

    if (0 != TMR_enqueue(&tmr, &ciStopEvent, millis()+scaleCmdDuration(t1))) {
      Error(ERR_TMR_ENQUEUE);
    }

    return CI_R_OK;
}

int handleCmdLT(uint8_t data[CI_MAX_DATA])
{
    uint8_t t1 = data[0];

    if (0 == t1) {
      t1 = QUARTER_TURN_DURATION;
    }

    changeSpeed(-MIN_MOTOR_SPEED, MIN_MOTOR_SPEED);

    if (0 != TMR_enqueue(&tmr, &ciStopEvent, millis()+scaleCmdDuration(t1))) {
      Error(ERR_TMR_ENQUEUE);
    }

    return CI_R_OK;
}

void handleCIStop(EVT_Event_t *e)
{
    if (EVT_TYPE_CI_STOP != e->type) {
      return;
    }

    changeSpeed(0, 0);
}

int handleCmdSet(uint8_t data[CI_MAX_DATA])
{
    uint8_t var = data[0];

    uint16_t val = data[2] << 8 | data[3];

    if (0 != TBL_set(&tbl, var, val)) {
      Error(ERR_TBL_SET);
      return CI_R_ERR_FAILED;
    }

    return CI_R_OK;
}

int handleCmdSnap(uint8_t data[CI_MAX_DATA])
{
    return CI_R_OK;
}