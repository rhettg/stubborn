#ifndef EVT_h
#define EVT_h
#include <stdint.h>

#define EVT_MAX_HANDLERS 8 

typedef struct {
    uint8_t type;
} EVT_Event_t;

typedef void (*notify_func_t)(EVT_Event_t *event);

typedef struct {
    notify_func_t handlers[EVT_MAX_HANDLERS];
} EVT_t;

void EVT_init(EVT_t *evt);
int EVT_notify(EVT_t *evt, EVT_Event_t *event);
int EVT_subscribe(EVT_t *evt, notify_func_t notify);
#endif