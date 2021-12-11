
#include <stdint.h>
#include <string.h>
#include "ci.h"

int CI_prepare_send(CI_t *ci, uint8_t cmd, uint8_t data[CI_MAX_DATA], unsigned long send_at)
{
    CI_Ready_Event_t ready_event = {{0}};
    ready_event.event.type = CI_EVT_TYPE_READY;

    ci->current.cmd = cmd;
    memcpy(ci->current.data, data, CI_MAX_DATA);
    ci->current.cmd_num++;
    ci->current.result = 0;

    ready_event.cmd = &ci->current;
    EVT_notify(ci->evt, (EVT_Event_t *)&ready_event);

    return 0;
}

int CI_ack(CI_t *ci, unsigned long cmd_num, uint8_t result, unsigned long ack_at)
{
    CI_Ack_Event_t ack_event = {{0}};
    ack_event.event.type = CI_EVT_TYPE_ACK;

    // TODO: we're not doing anything to ensure we've matched up requests with
    // the responses.  Only assuming they will match up sequentially. There was
    // code here to do it but I've ripped it out for now. We'll need a more
    // sophisticated message that includes our own token to match them up. If necessary.
    ci->current.result = result;

    ack_event.cmd = &ci->current;

    EVT_notify(ci->evt, (EVT_Event_t *)&ack_event);

    return 0;
}

int CI_ingest(CI_t *ci, uint16_t seq_num, uint8_t cmd, uint8_t data[CI_MAX_DATA])
{
    if (cmd > CI_MAX_CMDS) {
        return CI_R_ERR_NOT_FOUND;
    }

    if (NULL == ci->handlers[cmd]) {
        // This error differs from a standard -<value> to optimize passing the
        // value back to the sending CI system.
        return CI_R_ERR_NOT_FOUND;
    }

    // This would be better to require strictly increasing sequence numbers, but
    // we'll need some way to reset the CI system when a channel is restarted
    // (like when the ground station restarts)
    //
    // As it stands we still have a problem because if the ground station only
    // ever sends a single cmd and then restarts, we'll always think it's a
    // resend.
    // TODO: fix this
    if (seq_num != ci->current.cmd_num) {
        ci->current.cmd = cmd;

        memcpy(ci->current.data, data, CI_MAX_DATA);

        ci->current.cmd_num = seq_num;
        ci->current.result = ci->handlers[cmd](data);
    }

    return ci->current.result;
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