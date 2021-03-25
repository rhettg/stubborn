#ifndef TO_h
#define TO_h

#include <stdint.h>

enum PARAM{
    ERROR=0,
    MILLIS=1
};

struct TO_Object {
    enum PARAM param;
    uint32_t   data;
};

struct TO {
    struct TO_Object *objects;
    int count;
};

int TO_init(struct TO *to, int count);
//int TO_set(struct TO *to, int param, uint8_t value);
int TO_set(struct TO *to, int param, uint32_t value);
void TO_encode(struct TO *to, uint8_t **data, size_t *size);
#endif