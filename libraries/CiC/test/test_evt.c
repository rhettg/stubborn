#include <stdio.h>
#include <stdlib.h>
#include "minunit.h"
#include "mock_arduino.h"
#include "evt.h"
 
int tests_run = 0;

#define EVENT_TYPE_A 1
#define EVENT_TYPE_B 2

int notified = 0;
void notify(EVT_Event_t *evt)
{
    if (EVENT_TYPE_A != evt->type) {
        return;
    }

    notified++;
}

static char * test_subscribe() {
    EVT_t evt;
    EVT_init(&evt);

    EVT_Event_t aEvent;
    aEvent.type = EVENT_TYPE_A;

    EVT_Event_t bEvent;
    bEvent.type = EVENT_TYPE_B;

    EVT_notify(&evt, &aEvent);

    EVT_subscribe(&evt, &notify);
    notified = 0;

    EVT_notify(&evt, &bEvent);
    mu_assert("should not have notified ", notified == 0);

    EVT_notify(&evt, &aEvent);
    mu_assert("should have notified ", notified == 1);

    return 0;
}

static char * all_tests() {
    mu_run_test(test_subscribe);
    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();

    if (result != 0) {
        printf("test_evt: %s\n", result);
    }
    else {
        printf("test_evt: passed\n");
    }

    printf("test_evt: ran %d tests\n", tests_run);

    return result != 0;
}

