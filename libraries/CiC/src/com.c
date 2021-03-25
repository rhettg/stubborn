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
    if (length < sizeof(COM_Frame_t)) {
        return 0;
    }

    COM_Frame_t *frame = (COM_Frame_t *)data;
    if (COM_VERSION != COM_version(frame)) {
        return -1;
    }

    COM_CI_Event_t   ci_evt;
    COM_CI_R_Event_t ci_r_evt;
    COM_TO_Event_t   to_evt;

    switch (COM_type(frame)) {
        case COM_TYPE_CI:
            ci_evt.frame = (COM_CI_Frame_t *)data;
            ci_evt.data = data + sizeof(COM_CI_Frame_t);
            ci_evt.length = length - sizeof(COM_CI_Frame_t);
            EVT_notify(com->evt, (EVT_Event_t *)&ci_evt);
            return 0;
        case COM_TYPE_CI_R:
            ci_r_evt.frame = (COM_CI_R_Frame_t *)data;
            EVT_notify(com->evt, (EVT_Event_t *)&ci_r_evt);
            return 0;
        case COM_TYPE_TO:
            to_evt.data = data;
            to_evt.length = length;
            EVT_notify(com->evt, (EVT_Event_t *)&to_evt);
            return 0;
    }

    return -1;
}

int COM_send_ci(COM_t *com, uint8_t cmd, uint8_t cmd_num, uint8_t *data, size_t length)
{
    COM_Data_Event_t evt;
    evt.evt.type = COM_EVT_TYPE_CI;

    COM_CI_Frame_t *frame = (COM_CI_Frame_t *)com->data_buf;
    frame->cmd = cmd;
    frame->cmd_num = cmd_num;
    if (length > sizeof(com->data_buf)-sizeof(frame)) {
        return -1;
    }

    memcpy(com->data_buf+sizeof(frame), data, length);
    EVT_notify(com->evt, (EVT_Event_t *)&evt);

    return 0;
}

int COM_send_ci_r(COM_t *com, uint8_t cmd_num, uint8_t result)
{
    COM_Data_Event_t evt;
    evt.evt.type = COM_EVT_TYPE_CI_R;

    COM_CI_R_Frame_t *frame = (COM_CI_R_Frame_t *)com->data_buf;
    frame->cmd_num = cmd_num;
    EVT_notify(com->evt, (EVT_Event_t *)&evt);

    return 0;
}

int COM_send_to(COM_t *com, uint8_t *data, size_t length)
{
    COM_Data_Event_t evt;
    evt.evt.type = COM_EVT_TYPE_TO;

    evt.data = data;
    evt.length = length;

    EVT_notify(com->evt, (EVT_Event_t *)&evt);

    return 0;
}