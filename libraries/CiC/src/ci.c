
#include <stdint.h>
#include <string.h>
#include "ci.h"

int CI_prepare_send(CI_t *ci, uint8_t cmd, uint8_t data[CI_MAX_DATA], unsigned long send_at)
{
    CI_Ready_Event_t ready_event = {{0}};
    ready_event.event.type = CI_EVT_TYPE_READY;

    if (ci->current.cmd > 0 && ci->current.ack_at == 0) {
        return -1;
    }

    ci->current.cmd = cmd;
    memcpy(ci->current.data, data, CI_MAX_DATA);
    ci->current.send_at = send_at;

    ci->current.cmd_num++;

    ci->current.ack_at = 0;
    ci->current.result = 0;

    ready_event.cmd = &ci->current;
    EVT_notify(ci->evt, (EVT_Event_t *)&ready_event);

    return 0;
}

int CI_ack(CI_t *ci, unsigned long cmd_num, uint8_t result, unsigned long ack_at)
{
    CI_Ack_Event_t ack_event = {{0}};
    ack_event.event.type = CI_EVT_TYPE_ACK;

    if (cmd_num != ci->current.cmd_num) {
        return -1;
    }

    if (0 != ci->current.ack_at) {
        return -2;
    }

    ci->current.ack_at = ack_at;
    ci->current.result = result;

    ack_event.cmd = &ci->current;

    EVT_notify(ci->evt, (EVT_Event_t *)&ack_event);

    return 0;
}

int CI_ingest(CI_t *ci, uint8_t cmd, uint8_t data[CI_MAX_DATA])
{
    if (cmd > CI_MAX_CMDS) {
        return -1;
    }

    if (NULL == ci->handlers[cmd]) {
        // This error differs from a standard -<value> to optimize passing the
        // value back to the sending CI system.
        return CI_R_ERR_NOT_FOUND;
    }

    return ci->handlers[cmd](data);
}

int CI_register(CI_t *ci, uint8_t cmd, int(*handler)(uint8_t[CI_MAX_DATA]))
{
    if (cmd > CI_MAX_CMDS) {
        return -1;
    }

    if (NULL != ci->handlers[cmd]) {
        return -2;
    }

    ci->handlers[cmd] = handler;
    return 0;
}