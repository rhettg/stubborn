#include <stdlib.h>
#include "evt.h"

void EVT_init(EVT_t *evt)
{
    for (int n = 0; n < EVT_MAX_HANDLERS; n++) {
        evt->handlers[n] = NULL;
    }
}

int EVT_notify(EVT_t *evt, EVT_Event_t *event)
{
    for (int n = 0; n < EVT_MAX_HANDLERS; n++) {
        if (evt->handlers[n] != NULL) {
            evt->handlers[n](event);
        }
    }

    return 0;
}

int EVT_subscribe(EVT_t *evt, notify_func_t notify)
{
    for (int n = 0; n < EVT_MAX_HANDLERS; n++) {
        if (evt->handlers[n] == NULL) {
            evt->handlers[n] = notify;
            return 0;
        }
    }

    return -1;
}