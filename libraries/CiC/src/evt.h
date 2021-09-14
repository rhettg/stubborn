#ifndef EVT_h
#define EVT_h
#include <stdint.h>

// EVT_MAX_HANDLERS defines the size of the handlers array used for registering
// event handlers. If more handlers are needed, increase this number.
#ifndef EVT_MAX_HANDLERS
#define EVT_MAX_HANDLERS 10
#endif

typedef struct {
    uint8_t type;
} EVT_Event_t;

// notify_func_t defines functions that act as event receivers.
typedef void (*notify_func_t)(EVT_Event_t *event);

// EVT_t defines the Event Subsystem. 
// The system consists of a set of event handlers and functions for distributing events.
typedef struct {
    notify_func_t handlers[EVT_MAX_HANDLERS];
} EVT_t;

/** 
 * EVT_init initializes the event structure.
 * 
 * TODO: I think this should be unnecessary if we relied on initializing structs
 * to 0. I don't know how common that pattern is.
 * 
 * @returns void
 */
void EVT_init(EVT_t *evt);

/** 
 * EVT_notify accepts an event and distributes it to all registered event handlers.
 * 
 * @returns 0 on success
 */
int EVT_notify(EVT_t *evt, EVT_Event_t *event);

/** 
 * EVT_subscribe registers an event handler.
 * 
 * @returns 0 on success.
 * @returns -1 if there are insufficient handler slots available.
 */
int EVT_subscribe(EVT_t *evt, notify_func_t notify);
#endif