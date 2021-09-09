#ifndef COM_h
#define COM_h
#include <stdint.h>
#include <stdlib.h>
#include "evt.h"
#include "tmr.h"

#ifndef htons
// From https://github.com/arduino-libraries/Ethernet/blob/7c32bbe146bbe762093e4f021c3b12d7bf8d1629/src/utility/w5100.h
// The host order of the Arduino platform is little endian.
// Sometimes it is desired to convert to big endian (or
// network order)
#define htons(x) ( ((( x )&0xFF)<<8) | ((( x )&0xFF00)>>8) )
#define ntohs(x) htons(x)
#endif

// COM has two types of communication patterns:
//  * Request / Reply - Each request is expected to have a reply
//  * Broadcast -  No reply expected
#define COM_TYPE_REQ         1
#define COM_TYPE_REPLY       2
#define COM_TYPE_BROADCAST   3

#define COM_VERSION  2

// How long to wait after receiving a packet to allow replying.
// This is necessary to deal with single-duplex radios that need to be switched
// into receive mode.
#define COM_SEND_DELAY 1000

#define COM_SEND_RETRY 5000

/*
 * COM_Frame_t is the header frame for all COM messages. 
 * 
 * The header consists of a version and message type packed into a single uint8. 
 */
typedef struct {
    uint8_t header;
    uint8_t channel;
    uint16_t seq_num;
} COM_Frame_t;

// Maximum length for the communications buffer. This is set based on how large
// the physical communication system can send and receive.
#define COM_MAX_LENGTH 60

// Event to indicate we should consider sending out the payload
#define COM_EVT_TYPE_DISPATCH 13

typedef struct COM COM_t;

typedef struct {
    EVT_Event_t event;
    COM_t *com;
} COM_Dispatch_Event_t;

/*
 * COM_t is the communications component.
 * 
 * It requires a pointer to an EVT_t component to dispatch events based on received data.
 * The data buffer is used for sending new messages.
 */
typedef struct COM {
    EVT_t   *evt;
    TMR_t   *tmr;

    uint16_t seq_num;

    // recv_at marks when data was last received from our radio
    // This is helpful because we need to be friendly with our peers by not sending packets before their radio
    // is ready.
    unsigned long last_recv_at;

    COM_Dispatch_Event_t dispatch_event;

    uint8_t msg_type;
    size_t data_len;
    uint8_t data_buf[COM_MAX_LENGTH];
} COM_t;

// Event to indicate data is available for sending via communications device.
#define COM_EVT_TYPE_DATA 10

typedef struct {
    EVT_Event_t event;
    uint8_t *data;
    size_t length;
} COM_Data_Event_t;

#define COM_EVT_TYPE_MSG   11

typedef struct {
    EVT_Event_t event;
    uint8_t msg_type;
    uint8_t channel;
    uint16_t seq_num;
    size_t length;
    uint8_t *data;
} COM_Msg_Event_t;

/*
 * COM_init initializes the communications structure with the dependencies.
 *
 * @param com The COM_t component to initialize.
 * @param evt The EVT_t subsystem to dispatch events to.
 * @param tmr The TMR_t subsystem for setting timers.
 * 
 */
void COM_init(COM_t *com, EVT_t *evt, TMR_t *tmr);

/*
 * COM_notify should be called to receive dispatch events.
 *
 *     EVT_subscribe(&evt, &COM_notify)
 * 
 * This is necessary to support retries and delayed sending.
 *
 */
void COM_notify(EVT_Event_t *event);

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
 * @param now Current time (milliseconds)
*/
int COM_recv(COM_t *com, uint8_t *data, size_t length, unsigned long now);

/**
 * COM_send builds and emits a communications packet
 * 
 * This method uses the internal buffer of the COM_t system which will store
 * the data until completion of the resulting COM_EVT_TYPE_DATA.
 * 
 * @param com The COM_t component to use.
 * @param msg_type The type of message to send (COM_TYPE_*)
 * @param channel The channel identifier
 * @param data Pointer to data for the command.
 * @param length Length of data for the command.
 * @param now Current time (milliseconds)
 * 
 * @return 0 on ok, -1 on error like insufficent buffer space.
 */
int COM_send(COM_t *com, uint8_t msg_type, uint8_t channel, uint8_t *data, size_t length, unsigned long now);

/**
 * COM_send_reply builds and emits a communications packet in reply to a request.
 * 
 * This method uses the internal buffer of the COM_t system which will store
 * the data until completion of the resulting COM_EVT_TYPE_DATA.
 * 
 * @param com The COM_t component to use.
 * @param channel The channel identifier
 * @param seq_num The message sequence number to reply to
 * @param data Pointer to data for the command.
 * @param length Length of data for the command.
 * @param now Current time (milliseconds)
 * 
 * @return 0 on ok, -1 on error like insufficent buffer space.
 */
int COM_send_reply(COM_t *com, uint8_t channel, uint16_t seq_num, uint8_t *data, size_t length, unsigned long now);

#endif