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

/*
 * COM_Frame_t is the header frame for all COM messages. 
 * 
 * The header consists of a version and message type packed into a single uint8. 
 */
typedef struct {
    uint8_t header;
} COM_Frame_t;

typedef struct {
    uint8_t type;
    uint8_t *data;
    size_t length;
} COM_Msg_t;

// Maximum length for the communications buffer. This is set based on how large
// the physical communication system can send and receive.
#define COM_MAX_LENGTH 60

/*
 * COM_t is the communications component.
 * 
 * It requires a pointer to an EVT_t component to dispatch events based on received data.
 * The data buffer is used for sending new messages.
 */
typedef struct {
    EVT_t   *evt;
    uint8_t data_buf[COM_MAX_LENGTH];
    size_t data_len;
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

// Event to indicate data is available for sending via communications device.
#define COM_EVT_TYPE_DATA 10

// Event to indicate a message of type COM_TYPE_CI has been received.
#define COM_EVT_TYPE_CI   11

// Event to indicate a message of type COM_TYPE_CI_R has been received.
#define COM_EVT_TYPE_CI_R 12 

// Event to indicate a message of type COM_TYPE_TO has been received.
#define COM_EVT_TYPE_TO   13 

/**
 *  COM_recv ingests raw communication data into the com system.
 *
 * Typically this is used when data is received via some radio system. The raw
 * bytes are expected to be COM structured packets which this system will
 * decode and emit the appropriate events.
 * 
 * Data will not be copied out of the provided data buffer so it is expected to be
 * available for the duration of the call and will be re-used by any event handlers.
 * 
 * @param com The COM_t component to use.
 * @param data Pointer to the data buffer.
 * @param length Length of received data.
*/
int COM_recv(COM_t *com, uint8_t *data, size_t length);

/**
 * COM_send_ci builds and emits a communications packet of type COM_TYPE_CI
 * (command ingest).
 * 
 * This method uses the internal buffer of the COM_t system which will store
 * the data until completion of the resulting COM_EVT_TYPE_DATA.
 * 
 * @param com The COM_t component to use.
 * @param cmd The type of CI command to encode.
 * @param cmd_num The sequence number for the CI command.
 * @param data Pointer to data for the command.
 * @param length Length of data for the command.
 * 
 * @return 0 on ok, -1 on error like insufficent buffer space.
 * 
 * \sa ci.h
 */
int COM_send_ci(COM_t *com, uint8_t cmd, uint8_t cmd_num, uint8_t *data, size_t length);

/**
 * COM_send_ci_r builds and emits a communications packet of type COM_TYPE_CI_R
 * (command ingest response).
 * 
 * This method uses the internal buffer of the COM_t system which will store
 * the data until completion of the resulting COM_EVT_TYPE_DATA.
 * 
 * @param com The COM_t component to use.
 * @param cmd_num The sequence number for the CI command.
 * @param result Result of command operation.
 * 
 * @return 0 on ok, -1 on error like insufficent buffer space.
 * 
 * \sa ci.h
 */
int COM_send_ci_r(COM_t *com, uint8_t cmd_num, uint8_t result);

/**
 * COM_send_to builds and emits a communications packet of type COM_TYPE_TO
 * (telemetry output).
 * 
 * This method uses the internal buffer of the COM_t system which will store
 * the data until completion of the resulting COM_EVT_TYPE_DATA.
 * 
 * @param com The COM_t component to use.
 * @param data Pointer to data for the command.
 * @param length Length of data for the command.
 * 
 * @return 0 on ok, -1 on error like insufficent buffer space.
 * 
 * \sa to.h
 */
int COM_send_to(COM_t *com, uint8_t *data, size_t length);


/**
 * COM_send_retry retries the last sent packet.
 *
 * This is maybe temporary until it's automated? Should it live within COM?
 */
int COM_send_retry(COM_t *com);

#endif