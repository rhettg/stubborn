#include <string.h>
#include "com.h"

uint8_t COM_version(COM_Frame_t *frame) 
{
    return frame->header >> 4;
}

uint8_t COM_type(COM_Frame_t *frame)
{
    return frame->header & 0x0F;
}

static uint8_t encode_header(uint8_t type)
{
    return COM_VERSION << 4 | type;
}

// dispatch_send is a helper that sends the payload as a COM_Data_Event_t
// A given payload may be sent more than once if a reply is never received.
void dispatch_send(COM_t *com, unsigned long now)
{
    // If our payload is empty we have nothing to do. This was probably triggered by a left-over retry timer.
    if (0 == com->data_len) {
        return;
    }

    // Wait at least COM_SEND_DELAY since the last received packet to ensure somebody is listening.
    if (0 < com->last_recv_at && now - com->last_recv_at < COM_SEND_DELAY) {
        TMR_enqueue(com->tmr, &(com->dispatch_event), com->last_recv_at+COM_SEND_DELAY);
        return;
    }

    COM_Data_Event_t evt;
    evt.event.type = COM_EVT_TYPE_DATA;
    evt.data = com->data_buf;
    evt.length = com->data_len;

    EVT_notify(com->evt, (EVT_Event_t *)&evt);

    if (COM_TYPE_REQ == com->msg_type) {
        TMR_enqueue(com->tmr, &(com->dispatch_event), now+COM_SEND_RETRY);
    } else {
        com->data_len = 0;
    }
}

void COM_init(COM_t *com, EVT_t *evt, TMR_t *tmr)
{
    com->evt = evt;
    com->tmr = tmr;

    com->last_recv_at = 0;
    com->data_len = 0;

    com->dispatch_event.event.type = COM_EVT_TYPE_DISPATCH;
    com->dispatch_event.com = com;
}

void COM_notify(EVT_Event_t *event)
{
    if (COM_EVT_TYPE_DISPATCH != event->type) {
        return;
    }

    COM_Dispatch_Event_t *devent = (COM_Dispatch_Event_t *)event;

    dispatch_send(devent->com, TMR_now(devent->com->tmr));
}

int COM_recv(COM_t *com, uint8_t *data, size_t length, unsigned long now)
{
    uint8_t *payload = data;
    size_t payload_length = length;

    if (payload_length < sizeof(COM_Frame_t)) {
        return 0;
    }

    COM_Frame_t *frame = (COM_Frame_t *)payload;
    if (COM_VERSION != COM_version(frame)) {
        return -1;
    }

    com->last_recv_at = now;

    payload += sizeof(COM_Frame_t);
    payload_length -= sizeof(COM_Frame_t);

    if (COM_TYPE_REPLY == COM_type(frame) && ntohs(frame->seq_num) == com->seq_num) {
        // We got our reply, make sure we don't send it again.
        com->data_len = 0;
    }

    COM_Msg_Event_t   msg_evt;
    msg_evt.event.type = COM_EVT_TYPE_MSG;
    msg_evt.msg_type = COM_type(frame);
    msg_evt.seq_num = ntohs(frame->seq_num);
    msg_evt.channel = frame->channel;
    msg_evt.data = payload;
    msg_evt.length = payload_length;

    EVT_notify(com->evt, (EVT_Event_t *)&msg_evt);
    return 0;
}

int COM_send(COM_t *com, uint8_t msg_type, uint8_t channel, uint8_t *data, size_t length, unsigned long now)
{
    if (length + sizeof(COM_Frame_t) > sizeof(com->data_buf)) {
        return -1;
    }

    // Reset our current send packet
    com->seq_num++;
    com->data_len = 0;
    com->msg_type = msg_type;

    // and start building a new one
    uint8_t *payload = com->data_buf;

    COM_Frame_t *frame = (COM_Frame_t *)payload;

    frame->header = encode_header(msg_type);
    frame->channel = channel;
    frame->seq_num = htons(com->seq_num);

    com->data_len += sizeof(COM_Frame_t);
    payload += sizeof(COM_Frame_t);

    memcpy(payload, data, length);
    com->data_len += length;
    payload += length;

    dispatch_send(com, now);

    return 0;
}

int COM_send_reply(COM_t *com, uint8_t channel, uint16_t seq_num, uint8_t *data, size_t length, unsigned long now)
{
    if (length + sizeof(COM_Frame_t) > sizeof(com->data_buf)) {
        return -2;
    }

    com->data_len = 0;

    uint8_t *payload = com->data_buf;

    COM_Frame_t *frame = (COM_Frame_t *)payload;

    frame->header = encode_header(COM_TYPE_REPLY);
    frame->channel = channel;
    frame->seq_num = htons(seq_num);

    com->data_len += sizeof(COM_Frame_t);
    payload += sizeof(COM_Frame_t);

    memcpy(payload, data, length);
    com->data_len += length;
    payload += length;

    dispatch_send(com, now);

    return 0;
}
