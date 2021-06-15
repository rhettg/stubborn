#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minunit.h"
#include "mock_arduino.h"
#include "tbl.h"
 
int tests_run = 0;

uint16_t test_buffer[TBL_SIZE+1];

void clear()
{
    memset(test_buffer, 0, sizeof(test_buffer));
}

int save(const void *data, size_t data_len)
{
    memcpy(test_buffer, data, data_len);
    return 0;
}

int load(void *data, size_t data_len)
{
    memcpy(data, test_buffer, data_len);
    return 0;
}

int save_fail(const void *data, size_t data_len)
{
    return -1;
}

int load_fail(void *data, size_t data_len)
{
    return -1;
}

static char * test_tbl_fail() {
    TBL_t tbl;
    clear();

    TBL_init(&tbl, &save_fail, &load_fail);

    int ret = TBL_set_default(&tbl, 1, 0);
    mu_assert("set_default no error", ret == 0);

    ret = TBL_get(&tbl, 1, 0);
    mu_assert("get return error", ret == -1);

    ret = TBL_set(&tbl, 1, 0);
    mu_assert("set return errro", ret == -1);

    return 0;
}

static char * test_tbl() {
    TBL_t tbl;
    clear();
    TBL_init(&tbl, &save, &load);

    uint16_t value = 0;
    int ret = TBL_get(&tbl, 1, &value);
    mu_assert("get no error", ret == 0);
    mu_assert("get 0 val", value == 0);

    ret = TBL_set(&tbl, 1, 42);
    mu_assert("set no error", ret == 0);

    ret = TBL_get(&tbl, 1, &value);
    mu_assert("get no error", ret == 0);
    mu_assert("get 0 val", value == 42);

    // Now make sure it was actually saved
    TBL_t tblB;
    TBL_init(&tblB, &save, &load);
    ret = TBL_get(&tblB, 1, &value);
    mu_assert("get no error", ret == 0);
    mu_assert("get 0 val", value == 42);

    return 0;
}

static char * test_tbl_default() {
    TBL_t tbl;
    clear();
    TBL_init(&tbl, &save, &load);

    uint16_t value = 0;

    int ret = TBL_set_default(&tbl, 1, 255);
    mu_assert("set_default no error", ret == 0);

    ret = TBL_get(&tbl, 1, &value);
    mu_assert("get no error", ret == 0);
    mu_assert("get default val", value == 255);

    // It was just a default, shouldn't be saved
    TBL_t tblB;
    TBL_init(&tblB, &save, &load);
    ret = TBL_get(&tblB, 1, &value);
    mu_assert("get no error", ret == 0);
    mu_assert("get default 0 val", value == 0);

    // Set a second value for real, which should save it all out
    ret = TBL_set(&tbl, 2, 42);
    mu_assert("get no error", ret == 0);

    TBL_t tblC;
    TBL_init(&tblC, &save, &load);
    ret = TBL_get(&tblC, 1, &value);
    mu_assert("get no error", ret == 0);
    mu_assert("get default 0 val", value == 255);

    ret = TBL_get(&tblC, 2, &value);
    mu_assert("get no error", ret == 0);
    mu_assert("get default 0 val", value == 42);

    return 0;
}

static char * all_tests() {
    mu_run_test(test_tbl_fail);
    mu_run_test(test_tbl);
    mu_run_test(test_tbl_default);
    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();

    if (result != 0) {
        printf("test_tbl: %s\n", result);
    }
    else {
        printf("test_tbl: passed\n");
    }

    printf("test_tbl: ran %d tests\n", tests_run);

    return result != 0;
}

