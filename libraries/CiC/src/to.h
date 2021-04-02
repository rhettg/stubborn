#ifndef TO_h
#define TO_h

#include <stdint.h>

#define TO_MAX_PARAMS 16

typedef struct {
    uint8_t  param;
    uint32_t data;
} TO_Object_t;

typedef struct {
    TO_Object_t objects[TO_MAX_PARAMS];
} TO_t;

int TO_init(TO_t *to);
int TO_set(TO_t *to, uint8_t param, uint32_t value);
size_t TO_encode(TO_t *to, uint8_t *buf, size_t size);
#endif