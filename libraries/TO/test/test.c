#include <stdio.h>
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
    int r = TO_init(&to, 2);
    mu_assert("failed to init", r == 0);

    uint8_t v = 42;

    r = TO_set(&to, 0, v);
    mu_assert("failed to set 0", r == 0);

    r = TO_set(&to, 4, v);
    mu_assert("should have failed to set 4", r == -1);

    return 0;
}

static char * test_empty_encode() {
    struct TO to;
    int r = TO_init(&to, 0);
    mu_assert("failed to init", r == 0);

    uint8_t *data;
    size_t size;

    TO_encode(&to, &data, &size);
    mu_assert("failed to set size", size == 0);

    return 0;
}

static char * test_encode() {
    struct TO to;
    int r = TO_init(&to, 2);
    mu_assert("failed to init", r == 0);

    uint8_t v = 42;

    r = TO_set(&to, 0, v);
    mu_assert("failed to set 0", r == 0);

    uint8_t *data;
    size_t size;

    TO_encode(&to, &data, &size);
    mu_assert("failed to set data", data != NULL);
    mu_assert("failed to set size", size > 0);

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
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}

