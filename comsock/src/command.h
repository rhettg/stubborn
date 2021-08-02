#ifndef COMMAND_H
#define COMMAND_H
#include <stdlib.h>
#include "ci.h"

#define ERR_UNKNOWN  1
#define ERR_CMD      2

#define ERR_TO_SET   10

#define ERR_COM_SEND 20
#define ERR_COM_RECV 21

#define ERR_CI_SEND_FAIL 30
#define ERR_CI_ACK_FAIL  31

int parse_ci_command(CI_t *ci, char *s);
unsigned long millis();

#endif