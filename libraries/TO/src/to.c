#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "to.h"

int TO_init(struct TO *to, int count)
{
    if (to == NULL) {
        return -1;
    }

    to->objects = malloc(count * sizeof(struct TO_Object));
    if (to->objects == NULL) {
        return -1;
    }

    memset(to->objects, 0, count * sizeof(struct TO_Object));

    to->count = count;
    return 0;
}

int TO_set(struct TO *to, uint8_t param, uint32_t value)
{
    for (int n = 0; n < to->count; n++) {
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

size_t TO_encode(struct TO *to, uint8_t *buf, size_t size)
{
    size_t available = size;

    for (int n = 0; n < to->count; n++) {
        if (to->objects[n].param == 0) {
            continue;
        }

        if (available < sizeof(struct TO_Object)) {
            continue;
        }

        memcpy(buf, &(to->objects[n]), sizeof(struct TO_Object));
        buf += sizeof(struct TO_Object);
        available -= sizeof(struct TO_Object);
    }

    return size - available;
}