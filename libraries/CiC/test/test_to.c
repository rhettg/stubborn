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
    struct TO to;
    int r = TO_init(&to, 5);
    mu_assert("failed to init", r == 0);
    return 0;
}

static char * test_set() {
    struct TO to;
    int r = TO_init(&to, 1);
    mu_assert("failed to init", r == 0);

    uint8_t v = 42;

    r = TO_set(&to, 1, v);
    mu_assert("failed to set 0", r == 0);

    r = TO_set(&to, 4, v);
    mu_assert("should have failed to set 4", r == -1);

    return 0;
}

static char * test_empty_encode() {
    struct TO to;
    int r = TO_init(&to, 0);
    mu_assert("failed to init", r == 0);

    uint8_t *data = NULL;
    size_t size = 0;

    int encoded = TO_encode(&to, data, size);
    mu_assert("test_empty_encode: failed to set size", encoded == 0);

    return 0;
}

static char * test_encode() {
    struct TO to;
    int r = TO_init(&to, 2);
    mu_assert("failed to init", r == 0);

    uint8_t v = 42;

    r = TO_set(&to, 1, v);
    mu_assert("failed to set 0", r == 0);

    uint8_t *data = malloc(1024);

    int encoded = TO_encode(&to, data, 1024);
    //mu_assert("failed to set data", data != NULL);
    mu_assert("test_encode: failed to set size", encoded > 0);

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

