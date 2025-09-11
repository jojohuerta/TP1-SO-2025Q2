#ifndef VIEW_H
#define VIEW_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "shmConstants.h"
#include "shareMemory.h"

typedef enum {
    PLY1_RED =31,
    PLY2_BLUE =34,
    PLY3_GREEN =32,
    PLY4_YELLOW =33,
    PLY5_ORANGE =93,
    PLY6_PURPLE =95,
    PLY7_CYAN =36,
    PLY8_MAGENTA =35,
    PLY9_BLACK =30
} PlayerColor;

void whoWon(boardGameState* shm_bgs);

#endif