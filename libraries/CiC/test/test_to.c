#include <stdio.h>
#include <stdlib.h>
#include "minunit.h"
#include "mock_arduino.h"
#include "to.h"
 
int tests_run = 0;

/*
int foo = 7;
int bar = 4;

static char * test_foo() {
    mu_assert("error, foo != 7", foo == 7);
    return 0;
}

static char * test_bar() {
    mu_assert("error, bar != 5", bar == 5);
    return 0;
}
*/

static char * test_init() {
    TO_t to;
    int r = TO_init(&to);
    mu_assert("failed to init", r == 0);
    return 0;
}

static char * test_set() {
    TO_t to;
    int r = TO_init(&to);
    mu_assert("failed to init", r == 0);

    uint8_t v = 42;

    for (int n = 1; n < TO_MAX_PARAMS + 1; n++) {
        r = TO_set(&to, n, v);
        mu_assert("failed to set 0", r == 0);
    }

    r = TO_set(&to, 128, v);
    mu_assert("should have failed to set 128", r == -1);

    return 0;
}

static char * test_empty_encode() {
    TO_t to;
    int r = TO_init(&to);
    mu_assert("failed to init", r == 0);

    uint8_t *data = NULL;
    size_t size = 0;

    int encoded = TO_encode(&to, data, size);
    mu_assert("test_empty_encode: failed to set size", encoded == 0);

    return 0;
}

static char * test_encode() {
    TO_t to;
    int r = TO_init(&to);
    mu_assert("failed to init", r == 0);

    uint8_t v = 42;

    r = TO_set(&to, 1, v);
    mu_assert("failed to set 0", r == 0);

    TO_set(&to, 2, 1024);

    uint8_t *data = malloc(1024);

    int encoded = TO_encode(&to, data, 1024);
    mu_assert("test_encode: wrong size", encoded == TO_OBJECT_SIZE*2);

    TO_t to2;
    TO_init(&to2);
    int decoded = TO_decode(&to2, data, encoded);
    mu_assert("test_encode: decoded wrong elements", 2 == decoded);

    mu_assert("test_encode: wrong param", to2.objects[0].param == 1);
    mu_assert("test_encode: wrong value", to2.objects[0].data == 42);

    mu_assert("test_encode: wrong param 2", to2.objects[1].param == 2);
    mu_assert("test_encode: wrong value 2", to2.objects[1].data == 1024);

    free(data);

    return 0;
}

static char * all_tests() {
    mu_run_test(test_init);
    mu_run_test(test_set);
    mu_run_test(test_empty_encode);
    mu_run_test(test_encode);
    //mu_run_test(test_bar);
    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
        printf("test_to: %s\n", result);
    }
    else {
        printf("test_to: passed\n");
    }
    printf("test_to: ran %d tests\n", tests_run);

    return result != 0;
}

