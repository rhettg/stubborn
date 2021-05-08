#ifndef TMR_h
#define TMR_h

#include "evt.h"

#define TMR_MAX_TIMERS 16

typedef struct {
   EVT_t   *evt;

   unsigned long wake_millis;
   unsigned long event_notify[TMR_MAX_TIMERS];
   EVT_Event_t * events[TMR_MAX_TIMERS];
} TMR_t;

int TMR_enqueue(TMR_t *tmr, EVT_Event_t *e, unsigned long notify_at);
int TMR_handle(TMR_t *tmr, unsigned long now);

#endif