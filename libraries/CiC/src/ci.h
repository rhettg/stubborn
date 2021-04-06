#ifndef CI_h
#define CI_h

#include "evt.h"

#define CI_CMD_NOOP  1
#define CI_CMD_CLEAR 2
#define CI_CMD_BOOM  3
#define CI_CMD_STOP  4
#define CI_CMD_FWD   5

#define CI_MAX_CMDS 10

#define CI_R_OK            0
#define CI_R_ERR_INVALID   1 
#define CI_R_ERR_NOT_FOUND 2
#define CI_R_ERR_FAILED    3

#define CI_MAX_DATA 4

typedef int (*handler_t)(uint8_t[CI_MAX_DATA]);

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

    handler_t handlers[CI_MAX_CMDS+1];

    Command_t current;
} CI_t;

#define CI_EVT_TYPE_READY   20
#define CI_EVT_TYPE_ACK     21
#define CI_EVT_TYPE_COMMAND 22

typedef struct {
    EVT_Event_t event;
    Command_t *cmd;
} CI_Ack_Event_t;

typedef struct {
    EVT_Event_t event;
    Command_t *cmd;
} CI_Ready_Event_t;

typedef struct {
  EVT_Event_t event;
  uint8_t cmd;
  uint8_t data[CI_MAX_DATA];

  unsigned long cmd_num;
} CI_Command_Event_t;

int CI_register(CI_t *ci, uint8_t cmd, int(*handler)(uint8_t[CI_MAX_DATA]));

int CI_prepare_send(CI_t *ci, uint8_t cmd, uint8_t data[CI_MAX_DATA], unsigned long send_at);
int CI_ack(CI_t *ci, unsigned long cmd_num, uint8_t result, unsigned long ack_at);

int CI_ingest(CI_t *ci, uint8_t cmd, uint8_t data[CI_MAX_DATA]);

#endif