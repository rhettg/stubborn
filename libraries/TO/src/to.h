#ifndef TO_h
#define TO_h

#include <stdint.h>

struct TO_Object {
    uint8_t  param;
    uint32_t data;
};

struct TO {
    struct TO_Object *objects;
    int count;
};

int TO_init(struct TO *to, int count);
//int TO_set(struct TO *to, int param, uint8_t value);
int TO_set(struct TO *to, uint8_t param, uint32_t value);
size_t TO_encode(struct TO *to, uint8_t *buf, size_t size);
#endif