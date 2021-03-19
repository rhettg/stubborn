#include <stdio.h>
#include <stdlib.h>
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

    to->count = count;
    return 0;
}

int TO_set(struct TO *to, int param, uint32_t value)
{
    if (param >= to->count) {
        return -1;
    }

    to->objects[param].data = value;
    return 0;
}