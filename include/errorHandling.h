#ifndef ERRORHANDLING_H
#define ERRORHANDLING_H

#include <errno.h>
#include <limits.h>
#include <string.h>

#define MAX_ITOA_LENGTH INT_MAX%10

#define STR_ERR_LENGTH 1024

#define errExit(msg) \
    do {                            \
    fprintf(stderr, "%s\n%s\n", msg, errno == 0 ? "" : strerror(errno));   \
    exit(EXIT_FAILURE);             \
    } while(0);

#endif // ERRORHANDLING_h