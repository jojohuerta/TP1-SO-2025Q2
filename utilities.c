#include "include/shmConstants.h"
#include "include/utilities.h"
#include <time.h>
#include <stdlib.h>

void interpretMovement(unsigned char mov, boardGameState *shm_bgs, int player) {
    char msg[100];
    switch (mov) {
        case 0:
            shm_bgs->players[player].y--;
            break;
        case 1:
            shm_bgs->players[player].y--;
            shm_bgs->players[player].x++;
            break;
        case 2:
            shm_bgs->players[player].x++;
            break;
        case 3:
            shm_bgs->players[player].x++;
            shm_bgs->players[player].y++;
            break;
        case 4:
            shm_bgs->players[player].y++;
            break;
        case 5:
            shm_bgs->players[player].y++;
            shm_bgs->players[player].x--;
            break;
        case 6:
            shm_bgs->players[player].x--;
            break;
        case 7:
            shm_bgs->players[player].y--;
            shm_bgs->players[player].x--;
            break;
        default:
            snprintf(msg, sizeof(msg), "Movimiento no valido. Se leyo: %d", mov);
            errExit(msg);
    }
}

void initRandom() {
    srand(time(NULL)); 
}

int getSquareValue() {
    return (rand() % 9);
}

unsigned char movAnalysis() {
    return (unsigned char)(rand() % 8);
}
