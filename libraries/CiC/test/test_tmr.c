#include <stdio.h>
#include <stdlib.h>
#include "minunit.h"
#include "mock_arduino.h"
#include "tmr.h"
 
#define EVENT_TYPE_A 1
#define EVENT_TYPE_B 2

int tests_run = 0;

int notifiedA = 0;
int notifiedB = 0;
void notify(EVT_Event_t *evt)
{
    switch (evt->type) {
        case EVENT_TYPE_A:
            notifiedA++;
            break;
        case EVENT_TYPE_B:
            notifiedB++;
            break;
    }
}

static char * test_handle() {
    EVT_t evt;
    EVT_init(&evt);

    TMR_t tmr = {0};
    tmr.evt = &evt;

    EVT_Event_t aEvent;
    aEvent.type = EVENT_TYPE_A;

    EVT_Event_t bEvent;
    bEvent.type = EVENT_TYPE_B;

    EVT_subscribe(&evt, &notify);

    mu_assert("should have succeeded", TMR_enqueue(&tmr, &aEvent, 1000) == 0);
    mu_assert("should have succeeded", TMR_enqueue(&tmr, &bEvent, 2000) == 0);

    mu_assert("should have handled", TMR_handle(&tmr, 500) == 0);
    mu_assert("should not have delivered event 1", notifiedA == 0);
    mu_assert("should not have delivered event 2", notifiedB == 0);

    mu_assert("should have handled", TMR_handle(&tmr, 1001) == 0);
    mu_assert("should have delivered event 3", notifiedA == 1);
    mu_assert("should not have delivered event 4", notifiedB == 0);


    // Do it again, should be empty
    mu_assert("should have handled", TMR_handle(&tmr, 1001) == 0);
    mu_assert("should not have delivered event 5", notifiedA == 1);
    mu_assert("should not have delivered event 6", notifiedB == 0);

    mu_assert("should have handled", TMR_handle(&tmr, 2001) == 0);
    mu_assert("should have delivered event 7", notifiedB == 1);

    return 0;
}

static char * all_tests() {
    mu_run_test(test_handle);
    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();

    if (result != 0) {
        printf("test_tmr: %s\n", result);
    }
    else {
        printf("test_tmr: passed\n");
    }

    printf("test_tmr: ran %d tests\n", tests_run);

    return result != 0;
}

