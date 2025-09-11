#ifndef UTILITIES_H
#define UTILITIES_H

#include <errno.h>
#include <limits.h>
#include <string.h>

#include "shmConstants.h"

#define MAX_ITOA_LENGTH INT_MAX%10

#define STR_ERR_LENGTH 1024
#define errExit(msg) \
    do {                            \
    fprintf(stderr, "%s\n%s\n", msg, errno == 0 ? "" : strerror(errno));   \
    exit(EXIT_FAILURE);             \
    } while(0);

int interpretMovement(unsigned char mov, boardGameState *shm_bgs, int player);
void initializeAllPlayers(boardGameState *shm_bgs, int playerCount, pid_t * playerPids);
unsigned char movAnalysis();

#endif // UTILITIES_H