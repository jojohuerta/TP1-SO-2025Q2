#ifndef ERRORHANDLING_H
#define ERRORHANDLING_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define STR_ERR_LENGTH 1024

// Print custom error message and errno message if it's not "Success"
static inline void errExit(const char *msg)
{
    fprintf(stderr, "%s\n%s\n", msg, (errno == 0) ? "" : strerror(errno));
    exit(EXIT_FAILURE);
}

#endif