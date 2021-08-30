#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "to.h"

int TO_init(TO_t *to)
{
    if (to == NULL) {
        return -1;
    }

    memset(to->objects, 0, TO_MAX_PARAMS * sizeof(TO_Object_t));

    return 0;
}

int TO_set(TO_t *to, uint8_t param, uint32_t value)
{
    for (int n = 0; n < TO_MAX_PARAMS; n++) {
        if (to->objects[n].param == 0) {
            to->objects[n].param = param;
        }

        if (to->objects[n].param == param) {
            to->objects[n].data = value;
            return 0;
        }
    }

    return -1;
}

size_t TO_encode(TO_t *to, uint8_t *buf, size_t size)
{
    size_t available = size;
    uint8_t *b = buf;

    for (int n = 0; n < TO_MAX_PARAMS; n++) {
        if (0 == to->objects[n].param) {
            continue;
        }

        if (available < TO_OBJECT_SIZE) {
            continue;
        }

        b[0] = to->objects[n].param;
        memcpy(b+1, &(to->objects[n].data), 4);

        b += TO_OBJECT_SIZE;
        available -= TO_OBJECT_SIZE;
    }

    return b - buf;
}

int TO_decode(TO_t *to, uint8_t *buf, size_t size)
{
    int bn = 0;
    int n = 0;
    while (bn+TO_OBJECT_SIZE <= size) {
        if (n >= TO_MAX_PARAMS) {
            return -1;
        }

        to->objects[n].param = buf[bn];

        memcpy((uint8_t *)(&to->objects[n].data), buf+bn+1, 4);

        bn += TO_OBJECT_SIZE;

        n++;
    }

    return n;
}