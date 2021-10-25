#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minunit.h"
#include "mock_arduino.h"
#include "evt.h"
#include "com.h"
 
int tests_run = 0;

COM_Data_Event_t data_event = {0};
COM_Msg_Event_t msg_event = {0};

static void reset_data_event() 
{
    data_event.length = 0;
    data_event.data = NULL;
}

static void reset_msg_event() 
{
    msg_event.channel = 0;
    msg_event.seq_num = 0;
    msg_event.length = 0;
    msg_event.data = NULL;
}

static void test_com_notify(EVT_Event_t *event)
{
    switch (event->type) {
        case COM_EVT_TYPE_DATA:
            memcpy(&data_event, event, sizeof(COM_Data_Event_t));
            break;
        case COM_EVT_TYPE_MSG:
            memcpy(&msg_event, event, sizeof(COM_Msg_Event_t));
            break;
        default:
            printf("unknown type %d\n", event->type);
    }
}

static char * test_req_reply() {
    EVT_t evt = {0};
    EVT_init(&evt);

    TMR_t tmr = {0};

    COM_t comA = {0};
    COM_init(&comA, &evt, &tmr);

    COM_t comB = {0};
    COM_init(&comB, &evt, &tmr);

    char payload[] = "Hello World";

    EVT_subscribe(&evt, &test_com_notify);

    reset_data_event();
    int ret = COM_send(&comA, COM_TYPE_REQ, 1, (uint8_t *)&payload, sizeof(payload), 0);
    mu_assert("bad send", ret == 0);

    mu_assert("length is 0", data_event.length > sizeof(payload));
    mu_assert("data is null", data_event.data != NULL);

    reset_msg_event();
    ret = COM_recv(&comB, data_event.data, data_event.length, 0);
    mu_assert("bad recv", ret == 0);

    mu_assert("wrong channel", 1 == msg_event.channel);

    // Send a reply
    reset_data_event();
    ret = COM_send_reply(&comB, msg_event.channel, msg_event.seq_num, (uint8_t *)&payload, sizeof(payload), 0);
    mu_assert("bad send_reply", ret == 0);
    mu_assert("length too small", data_event.length > sizeof(payload));
    mu_assert("data is null", data_event.data != NULL);

    reset_msg_event();
    ret = COM_recv(&comA, data_event.data, data_event.length, 0);
    mu_assert("bad recv", ret == 0);

    mu_assert("wrong type", COM_TYPE_REPLY == msg_event.msg_type);
    mu_assert("wrong channel", 1 == msg_event.channel);
    mu_assert("still waiting", 0 == comA.channels[1].data_len);

    return 0;
}

static char * test_delay_send() {
    EVT_t evt = {0};
    EVT_init(&evt);

    TMR_t tmr = {0};

    COM_t comA = {0};
    COM_init(&comA, &evt, &tmr);

    COM_t comB = {0};
    COM_init(&comB, &evt, &tmr);

    char payload[] = "Hello World";

    EVT_subscribe(&evt, &test_com_notify);

    reset_data_event();
    int ret = COM_send(&comA, COM_TYPE_REQ, 0, (uint8_t *)&payload, sizeof(payload), 1);
    mu_assert("bad send", ret == 0);
    mu_assert("notify not successful", data_event.length > 0);

    reset_msg_event();
    ret = COM_recv(&comB, data_event.data, data_event.length, 2);
    mu_assert("bad recv", ret == 0);
    mu_assert("no msg data", data_event.length > 0);

    // Send a reply, but it will be delayed
    reset_data_event();
    ret = COM_send_reply(&comB, msg_event.channel, msg_event.seq_num, (uint8_t *)&payload, sizeof(payload), 2);
    mu_assert("bad send_reply", ret == 0);
    mu_assert("notify not delayed", data_event.length == 0);

    tmr.now_millis = 3;
    COM_notify((EVT_Event_t *)&(comB.channels[0].dispatch_event));
    mu_assert("notify too soon", data_event.length == 0);

    tmr.now_millis = COM_SEND_DELAY+3;
    COM_notify((EVT_Event_t *)&(comB.channels[0].dispatch_event));

    mu_assert("notify too late", data_event.length > 0);

    return 0;
}


static char * all_tests() {
    mu_run_test(test_req_reply);
    mu_run_test(test_delay_send);
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

