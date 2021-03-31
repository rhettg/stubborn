#ifndef COM_h
#define COM_h
#include <stdint.h>
#include <stdlib.h>
#include "evt.h"

// Command Ingest
#define COM_TYPE_CI   1
// Command Response
#define COM_TYPE_CI_R 2
// Telemetry Output
#define COM_TYPE_TO   3

#define COM_VERSION  1

typedef struct {
    uint8_t header;
} COM_Frame_t;

typedef struct {
    uint8_t type;
    uint8_t *data;
    size_t length;
} COM_Msg_t;

#define COM_MAX_LENGTH 60

typedef struct {
    EVT_t   *evt;
    uint8_t data_buf[COM_MAX_LENGTH];
} COM_t;

typedef struct {
    uint8_t cmd;
    uint8_t cmd_num;
} COM_CI_Frame_t;

typedef struct {
    EVT_Event_t event;
    COM_CI_Frame_t *frame;
    uint8_t *data;
    size_t length;
} COM_CI_Event_t;

typedef struct {
    uint8_t cmd_num;
    uint8_t result;
} COM_CI_R_Frame_t;

typedef struct {
    EVT_Event_t event;
    COM_CI_R_Frame_t *frame;
} COM_CI_R_Event_t;

typedef struct {
    EVT_Event_t event;
    uint8_t *data;
    size_t length;
} COM_TO_Event_t;

typedef struct {
    EVT_Event_t event;
    uint8_t *data;
    size_t length;
} COM_Data_Event_t;

#define COM_EVT_TYPE_DATA 10
#define COM_EVT_TYPE_CI   11
#define COM_EVT_TYPE_CI_R 12 
#define COM_EVT_TYPE_TO   13 

int COM_recv(COM_t *com, uint8_t *data, size_t length);

int COM_send_ci(COM_t *com, uint8_t cmd, uint8_t cmd_num, uint8_t *data, size_t length);
int COM_send_ci_r(COM_t *com, uint8_t cmd_num, uint8_t result);
int COM_send_to(COM_t *com, uint8_t *data, size_t length);

//void COM_notify(EVT_Event_t *evt);
#endif