#ifndef CI_h
#define CI_h

#include "evt.h"

#define CI_CMD_NOOP  1
#define CI_CMD_CLEAR 2
#define CI_CMD_BOOM  3

#define CI_R_OK            0
#define CI_R_ERR_NOT_FOUND 1
#define CI_R_ERR_FAILED    2

#define CI_MAX_DATA 4

typedef struct {
  uint8_t cmd;
  uint8_t data[CI_MAX_DATA];

  unsigned long cmd_num;
  unsigned long send_at;

  unsigned long ack_at;
  uint8_t result;
} Command_t;

typedef struct {
    EVT_t *evt;

    Command_t current;
} CI_t;

#define CI_EVT_TYPE_READY 20
#define CI_EVT_TYPE_ACK   21

typedef struct {
    EVT_Event_t event;
    Command_t *cmd;
} CI_Ack_Event_t;

typedef struct {
    EVT_Event_t event;
    Command_t *cmd;
} CI_Ready_Event_t;

int CI_prepare_send(CI_t *ci, uint8_t cmd, uint8_t data[4], unsigned long send_at);
int CI_ack(CI_t *ci, unsigned long cmd_num, uint8_t result, unsigned long ack_at);

#endif