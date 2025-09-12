#include <stdio.h>
#include <unistd.h>

#include <math.h>

#include "../include/shmConstants.h"

#ifndef PI
#define PI 3.141592
#endif

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
    // Agrego los puntos
    shm_bgs->players[player].score += shm_bgs->boardStart[(newY * shm_bgs->boardWidth) + newX] ;

    // Registrar nuevo movimiento y contarlo como valido
    shm_bgs->players[player].x = newX;
    shm_bgs->players[player].y = newY;

    shm_bgs->boardStart[newPosIndex] = (-1) * player;    
    shm_bgs->players[player].validMovementRequests++;
    return 1;
}

void playerInitialization(int player, pid_t playerPid, int playerCount, boardGameState *shm_bgs){

    // Centro del tablero
    float cx = (shm_bgs->boardWidth - 1) / 2.0f;
    float cy = (shm_bgs->boardHeight - 1) / 2.0f;

    int x, y;

    if (playerCount == 1) {
        // Posicionar en el centro exacto del tablero
        x = (int)(cx + 0.5f);
        y = (int)(cy + 0.5f);
    } else {
        // Elegir un radio seguro que no nos acerque demasiado al borde
        float margin = 2.0f; // padding 
        float max_r_x = cx - margin;
        float max_r_y = cy - margin;
        float radius = fminf(max_r_x, max_r_y); // radio max posible

        // Angulo para el jugador (radianes)
        float angle = (2.0f * PI * player) / playerCount;

        // Posicion final del jugador en el circulo (0.5f para redondear)
        x = (int)(cx + radius * cosf(angle) + 0.5f);
        y = (int)(cy + radius * sinf(angle) + 0.5f);
    }

    // Asignar valores al jugador
    shm_bgs->players[player].x = x;
    shm_bgs->players[player].y = y;
    shm_bgs->players[player].score = 0;
    shm_bgs->players[player].isBlocked = 0;
    shm_bgs->players[player].invalidMovementRequests = 0;
    shm_bgs->players[player].validMovementRequests = 0;
    shm_bgs->players[player].processID = playerPid;

    // Marcar posición en el tablero
    shm_bgs->boardStart[(y * shm_bgs->boardWidth) + x] = (-1) * player;
}

void initializeAllPlayers(boardGameState *shm_bgs, int playerCount, pid_t * playerPids){
    for (int i = 0; i < playerCount; i++){
        playerInitialization(i, playerPids[i], playerCount, shm_bgs);
    }
}