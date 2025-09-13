#ifndef MAXITOALENGTH_H
#define MAXITOALENGTH_H

#include <limits.h>

#define MAX_ITOA_LENGTH max_itoa_length()

static inline int max_itoa_length()
{
    int i = 0, j = INT_MAX;
    while (j != 0)
    {
        i++;
        j /= 10;
    }
    return i;
}

#endif