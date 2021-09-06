#include <stdio.h>
#include <stdlib.h>
#include "minunit.h"
#include "mock_arduino.h"
#include "evt.h"
#include "com.h"
 
int tests_run = 0;

COM_Data_Event_t *data_event = NULL;
COM_Msg_Event_t *msg_event = NULL;

static void test_com_notify(EVT_Event_t *event)
{
    switch (event->type) {
        case COM_EVT_TYPE_DATA:
            data_event = (COM_Data_Event_t *)event;
            break;
        case COM_EVT_TYPE_MSG:
            msg_event = (COM_Msg_Event_t *)event;
            break;
        default:
            printf("unknown type %d\n", event->type);
    }
}

static char * test_req_reply() {
    EVT_t evt = {0};
    EVT_init(&evt);

    TMR_t tmr = {0};

    COM_t com = {0};
    COM_init(&com, &evt, &tmr);

    char payload[] = "Hello World";

    EVT_subscribe(&evt, &test_com_notify);

    int ret = COM_send(&com, COM_TYPE_REQ, 1, (uint8_t *)&payload, sizeof(payload), 0);
    mu_assert("bad send", ret == 0);
    mu_assert("notify not successful", data_event != NULL);

    mu_assert("length is 0", data_event->length > 0);
    mu_assert("data is null", data_event->data != NULL);

    ret = COM_recv(&com, data_event->data, data_event->length, 0);
    mu_assert("bad recv", ret == 0);

    mu_assert("recv not successful", msg_event != NULL);
    mu_assert("wrong channel", 1 == msg_event->channel);

    data_event = NULL;

    // Send a reply
    ret = COM_send_reply(&com, msg_event->channel, msg_event->seq_num, (uint8_t *)&payload, sizeof(payload), 0);
    mu_assert("bad send_reply", ret == 0);
    mu_assert("notify not successful", data_event != NULL);
    mu_assert("length is 0", data_event->length > 0);
    mu_assert("data is null", data_event->data != NULL);

    msg_event = NULL;

    ret = COM_recv(&com, data_event->data, data_event->length, 0);
    mu_assert("bad recv", ret == 0);

    mu_assert("recv not successful", msg_event != NULL);
    mu_assert("wrong type", COM_TYPE_REPLY == msg_event->msg_type);
    mu_assert("wrong channel", 1 == msg_event->channel);

    return 0;
}

/*
static char * test_delay_send() {
    EVT_t evt = {0};
    EVT_init(&evt);

    TMR_t tmr = {0};

    COM_t com = {0};
    COM_init(&com, &evt, &tmr);

    char payload[] = "Hello World";

    EVT_subscribe(&evt, &test_ci_notify);

    int ret = COM_send_ci(&com, 2, 1, (uint8_t *)&payload, sizeof(payload));
    mu_assert("bad send_ci", ret == 0);
    mu_assert("notify not successful", ci_notify_d_event != NULL);

    mu_assert("length is 0", ci_notify_d_event->length > 0);
    mu_assert("data is null", ci_notify_d_event->data != NULL);

    ret = COM_recv(&com, ci_notify_d_event->data, ci_notify_d_event->length, 0);
    mu_assert("bad recv", ret == 0);

    mu_assert("recv not successful", ci_notify_ci_event != NULL);

    return 0;
}
*/


static char * all_tests() {
    mu_run_test(test_req_reply);
    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
        printf("test_com: %s\n", result);
    }
    else {
        printf("test_com: passed\n");
    }
    printf("test_com: ran %d tests\n", tests_run);

    return result != 0;
}

