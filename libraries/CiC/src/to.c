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

        if (available < sizeof(TO_Object_t)) {
            continue;
        }

        memcpy(b, &(to->objects[n]), sizeof(TO_Object_t));
        b += sizeof(TO_Object_t);
        available -= sizeof(TO_Object_t);
    }

    return b - buf;
}