#include "tmr.h"
#include <stdio.h>

int TMR_enqueue(TMR_t *tmr, EVT_Event_t *e, unsigned long notify_at)
{
    if (0 == tmr) {
        return -1;
    }

    if (0 == e) {
        return -1;
    }

    for (int i = 0; i < TMR_MAX_TIMERS; i++) {
        if (tmr->event_notify[i] == 0) {
            tmr->event_notify[i] = notify_at;
            tmr->events[i] = e;

            if ((0 == tmr->wake_millis) || (notify_at < tmr->wake_millis)) {
                tmr->wake_millis = notify_at;
            }
            return 0;
        }
    }
    return -1;
}

int TMR_handle(TMR_t *tmr, unsigned long now)
{
    if (0 == tmr || 0 == tmr->evt) {
        return -1;
    }

    if (tmr->wake_millis == 0 || tmr->wake_millis > now) {
        return 0;
    }

    tmr->wake_millis = 0;
    tmr->now_millis = now;

    for (int i = 0; i < TMR_MAX_TIMERS; i++) {
        if (tmr->event_notify[i] == 0) {
            continue;
        }

        if (tmr->event_notify[i] < now) {
            EVT_notify(tmr->evt, tmr->events[i]);

            tmr->event_notify[i] = 0;
            tmr->events[i] = 0;
        } else {
            if ((0 == tmr->wake_millis) || (tmr->event_notify[i] < tmr->wake_millis)) {
                tmr->wake_millis = tmr->event_notify[i];
            }
        }
    }

    return 0;
}

unsigned long TMR_now(TMR_t *tmr)
{
    return tmr->now_millis;
}