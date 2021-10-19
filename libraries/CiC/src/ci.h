#ifndef CI_h
#define CI_h

#include "evt.h"

#define CI_CMD_NOOP  1
#define CI_CMD_CLEAR 2
#define CI_CMD_BOOM  3

#ifndef CI_MAX_CMDS
// TODO: How do I make these constants mission specific?
#define CI_MAX_CMDS 12
#endif

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

// A command is ready to send
#define CI_EVT_TYPE_READY   20

// A command as been ack'd
#define CI_EVT_TYPE_ACK     21

// A command has been received.
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

/*
 * CI_register associates the provided command handler with the specified command.
 * 
 * Each command can only have a single command which will handle any received command events.
 * 
 * @param ci CI_t component providing shared CI datastructures.
 * @param cmd The command type to set a handler for. Command types are typically mission specific.
 * @param handler The handler function to receive commands of this type.
 * 
 * @returns 0 if successful. 
 * @returns -1 if the command number is out of range.
 * @returns -2 if the handler has already been set.
 */
int CI_register(CI_t *ci, uint8_t cmd, int(*handler)(uint8_t[CI_MAX_DATA]));

/*
 * CI_prepare_send generates an event to indicate the command is ready to be sent.
 * 
 * Typically the event generated, CI_EVT_TYPE_READY will be handled by the COM system.
 * 
 * This method is expected to be used by a component that sends commands. For
 * example, a ground station.
 * 
 * TODO: This should likely directly depend on the COM system. This feels like
 * unnecessary indirection.
 * 
 * @param ci CI_t component providing shared CI datastructures.
 * @param cmd The command to send.
 * @param data data associated with the command.
 * @param send_at The current timestamp associated with the command. Typically millis().
 * 
 * @returns 0 if successful. 
 * @returns -1 for an error. Typically an invalid command.
 */
int CI_prepare_send(CI_t *ci, uint8_t cmd, uint8_t data[CI_MAX_DATA], unsigned long send_at);

/*
 * CI_ack acknowledges the specific command as processed by the recipient.
 * 
 * This method is expected to be used by a component that sends commands. For
 * example, a ground station.
 * 
 * @param ci CI_t component providing shared CI datastructures.
 * @param cmd_num The command sequence number to acknowledge.
 * @param result The result value for the command.
 * @param ack_at The current timestamp associated with the ack. Typically millis().
 * 
 * @returns 0 if successful. 
 * @returns -1 for an error. Invalid cmd_num.
 * @returns -2 for an error. Duplicate ack.
 */
int CI_ack(CI_t *ci, unsigned long cmd_num, uint8_t result, unsigned long ack_at);

/*
 * CI_ingest accepts command data and dispatches to appropriate handlers.
 * 
 * @param ci CI_t component providing shared CI datastructures.
 * @param cmd The command to dispatch.
 * @param data Data for the command.
 * 
 * @returns 0 if successful. 
 * @returns CI_R_ERR_NOT_FOUND to indicate no handler is available for the command.
 * @returns -1 for an error. Invalid cmd.
 */
int CI_ingest(CI_t *ci, uint8_t cmd, uint8_t data[CI_MAX_DATA]);

#endif