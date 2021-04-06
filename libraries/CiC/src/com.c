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

void COM_notify(EVT_Event_t *evt)
{
    /*
    switch (evt->type) {
        case COM_EVT_TYPE_DATA_IN:
            COM_notify_data_in((COM_DataIn_Event *)evt);
            break;
        case COM_EVT_TYPE_DATA_OUT:
            COM_notify_data_out((COM_DataOut_Event *)evt);
            break;
    }
    return;
    */
}

int COM_recv(COM_t *com, uint8_t *data, size_t length)
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

    payload += sizeof(COM_Frame_t);
    payload_length -= sizeof(COM_Frame_t);

    COM_CI_Event_t   ci_evt;
    ci_evt.event.type = COM_EVT_TYPE_CI;

    COM_CI_R_Event_t ci_r_evt;
    ci_r_evt.event.type = COM_EVT_TYPE_CI_R;

    COM_TO_Event_t   to_evt;
    to_evt.event.type = COM_EVT_TYPE_TO;

    switch (COM_type(frame)) {
        case COM_TYPE_CI:
            ci_evt.frame = (COM_CI_Frame_t *)payload;
            ci_evt.length = length - sizeof(COM_CI_Frame_t);
            if (ci_evt.length > 0) {
                ci_evt.data = payload + sizeof(COM_CI_Frame_t);
            } else {
                ci_evt.data = NULL;
            }

            EVT_notify(com->evt, (EVT_Event_t *)&ci_evt);
            return 0;
        case COM_TYPE_CI_R:
            ci_r_evt.frame = (COM_CI_R_Frame_t *)payload;
            EVT_notify(com->evt, (EVT_Event_t *)&ci_r_evt);
            return 0;
        case COM_TYPE_TO:
            to_evt.data = payload;
            to_evt.length = payload_length;
            EVT_notify(com->evt, (EVT_Event_t *)&to_evt);
            return 0;
    }

    return -2;
}

int COM_send_ci(COM_t *com, uint8_t cmd, uint8_t cmd_num, uint8_t *data, size_t length)
{
    COM_Data_Event_t evt;
    evt.event.type = COM_EVT_TYPE_DATA;
    evt.data = com->data_buf;
    evt.length = 0;

    uint8_t *payload = com->data_buf;

    COM_Frame_t *frame = (COM_Frame_t *)payload;
    frame->header = encode_header(COM_TYPE_CI);
    evt.length += sizeof(COM_Frame_t);
    payload += sizeof(COM_Frame_t);

    COM_CI_Frame_t *ci_frame = (COM_CI_Frame_t *)payload;
    ci_frame->cmd = cmd;
    ci_frame->cmd_num = cmd_num;
    evt.length += sizeof(COM_CI_Frame_t);
    payload += sizeof(COM_CI_Frame_t);

    if (evt.length + length > sizeof(com->data_buf)) {
        return -1;
    }

    memcpy(payload, data, length);
    evt.length += length;
    payload += length;

    EVT_notify(com->evt, (EVT_Event_t *)&evt);

    return 0;
}

int COM_send_ci_r(COM_t *com, uint8_t cmd_num, uint8_t result)
{
    COM_Data_Event_t evt;
    evt.event.type = COM_EVT_TYPE_DATA;
    evt.data = com->data_buf;
    evt.length = 0;

    uint8_t *payload = com->data_buf;

    COM_Frame_t *frame = (COM_Frame_t *)payload;
    frame->header = encode_header(COM_TYPE_CI_R);
    evt.length += sizeof(COM_Frame_t);
    payload += sizeof(COM_Frame_t);

    COM_CI_R_Frame_t *ci_r_frame = (COM_CI_R_Frame_t *)payload;
    ci_r_frame->cmd_num = cmd_num;
    ci_r_frame->result = result;
    evt.length += sizeof(COM_CI_R_Frame_t);
    payload += sizeof(COM_CI_R_Frame_t);

    EVT_notify(com->evt, (EVT_Event_t *)&evt);

    return 0;
}

int COM_send_to(COM_t *com, uint8_t *data, size_t length)
{
    COM_Data_Event_t evt;
    evt.event.type = COM_EVT_TYPE_DATA;
    evt.data = com->data_buf;
    evt.length = 0;

    uint8_t *payload = com->data_buf;

    COM_Frame_t *frame = (COM_Frame_t *)payload;
    frame->header = encode_header(COM_TYPE_TO);
    evt.length += sizeof(COM_Frame_t);
    payload += sizeof(COM_Frame_t);

    if (evt.length + length > sizeof(com->data_buf)) {
        return -1;
    }

    memcpy(payload, data, length);
    evt.length += length;
    payload += length;

    EVT_notify(com->evt, (EVT_Event_t *)&evt);

    return 0;
}