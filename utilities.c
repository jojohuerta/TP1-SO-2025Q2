#include "include/shmConstants.h"
#include "include/utilities.h"
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

int interpretMovement(unsigned char mov, boardGameState *shm_bgs, int player) {
    int dx = 0, dy = 0;

    switch (mov) {
        case 0: dy = -1; break; // Arriba
        case 1: dy = -1; dx = +1; break; // Arriba y derecha
        case 2: dx = +1; break; // Derecha
        case 3: dx = +1; dy = +1; break; // Abajo y derecha
        case 4: dy = +1; break; // Abajo
        case 5: dy = +1; dx = -1; break; // Abajo e izquierda
        case 6: dx = -1; break; // izquierda
        case 7: dx = -1; dy = -1; break; // Arriba e izquierda
        default:
            shm_bgs->players[player].invalidMovementRequests++;
            return 0;
    }

    int oldX = shm_bgs->players[player].x;
    int oldY = shm_bgs->players[player].y;
    int newX = oldX + dx;
    int newY = oldY + dy;

    printf("Jugador %d en (%d,%d) se quiere mover a (%d,%d)\n", player, oldX, oldY, newX, newY);

    // Validacion de si no se fue del tablero
    if (newX < 0 || newX >= shm_bgs->boardWidth || newY < 0 || newY >= shm_bgs->boardHeight) {
        shm_bgs->players[player].invalidMovementRequests++;
        return 0;
    }

    int newPosIndex = newX + newY * shm_bgs->boardWidth;

    // Validacion si la nueva casilla esta libre
    if (shm_bgs->boardStart[newPosIndex] <= 0) {
        shm_bgs->players[player].invalidMovementRequests++;
        return 0;
    }

    // Si llego hasta aca, el movimiento es válido

    // Registrar nuevo movimiento y contarlo como valido
    shm_bgs->players[player].x = newX;
    shm_bgs->players[player].y = newY;

    shm_bgs->boardStart[newPosIndex] = (-1) * player;    
    shm_bgs->players[player].validMovementRequests++;
    return 1;
}


void initRandom() {
    srand(time(NULL) * getpid()); 
}

int getSquareValue() {
    return (rand() % 8) + 1;
}

unsigned char movAnalysis() {
    return (unsigned char)(rand() % 8);
}
