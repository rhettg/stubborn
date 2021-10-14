#ifndef TMR_h
#define TMR_h

#include "evt.h"

// TMR_MAX_TIMERS defines the size of the timers array.
// If more timers are needed, increase this number at the cost of more memory usage.
#define TMR_MAX_TIMERS 16

/** 
 * TMR_t defines the TMR (Timer) subsystem. This helpful for scheduling events
 * in to be dispatched in the future.
 *
 * In practice TMR operates in millseconds but the times are really opaque long
 * integers and could be anything as long as they compare.
 */
typedef struct {
   EVT_t   *evt;

   unsigned long now_millis;                    // defines when now is for use by tmr dispatched event handlers;
   unsigned long wake_millis;                   // defines when the next event to be delivereed is scheduled.
   EVT_Event_t * events[TMR_MAX_TIMERS];        // array of events to delivery
   unsigned long event_notify[TMR_MAX_TIMERS];  // array of millisecond notify_at values for each event
} TMR_t;

/**
 * TMR_enqueue adds the provided event to be distributed at the indicated time.
 *
 * @param tmr TMR_t instance
 * @param e EVT_Event_t to enqueue
 * @param notify_at When to dispatch the event (milliseconds)
 */
int TMR_enqueue(TMR_t *tmr, EVT_Event_t *e, unsigned long notify_at);

/**
 * TMR_handle is the scheduling hook into TMR. It should be called frequently
 * to give TMR an opportunity to dispatch scheduled events.
 *
 * @param tmr TMR_t instance
 * @param now Current time (milliseconds)
 */
int TMR_handle(TMR_t *tmr, unsigned long now);

/**
 * TMR_now returns the current time as recorded when TMR_handle was called to dispatch an event.
 * This is useful for handlers who need to enqueue another event or otherwise deal with the current time.
 *
 * @param tmr TMR_t instance
 */
unsigned long TMR_now(TMR_t *tmr);

#endif