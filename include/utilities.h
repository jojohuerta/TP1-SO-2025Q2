#ifndef UTILITIES_H
#define UTILITIES_H

#include "shmConstants.h"

void interpretMovement(unsigned char mov, boardGameState *shm_bgs, int player);
void initRandom();
int getSquareValue();
unsigned char movAnalysis();

#endif // UTILITIES_H