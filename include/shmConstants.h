#ifndef SHMCONSTANTS_H
#define SHMCONSTANTS_H

#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define GAME_STATE_PATH "/game_state"
#define SYNC_STATE_PATH "/game_sync"

#define SYNC_STATE_SIZE sizeof(syncState)

typedef char bool;

typedef struct {
    sem_t view_print_pending_sem;       // El máster le indica a la vista que hay cambios por imprimir
    sem_t view_print_done_sem;          // La vista le indica al máster que terminó de imprimir
    sem_t game_state_starvation_mutex;  // Mutex para evitar inanición del máster al acceder al estado
    sem_t game_state_mutex;             // Mutex para el estado del juego                             
    sem_t reader_count_mutex;           // Mutex para la siguiente variable
    unsigned int reader_count;          // Cantidad de jugadores leyendo el estado     
    sem_t player_can_move_sem[9];       // Le indican a cada jugador que puede enviar 1 movimiento
} syncState;

typedef struct {
    char playerName[16];                    // Nombre del jugador
    unsigned int score;                     // Puntaje
    unsigned int validMovementRequests;     // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int invalidMovementRequests;   // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y;                    // Coordenadas x e y en el tablero
    pid_t processID;                        // Identificador de proceso
    bool isBlocked;                         // Indica si el jugador está bloqueado
} playerState;

typedef struct {
    unsigned short boardWidth;      // Ancho del tablero
    unsigned short boardHeight;     // Alto del tablero
    unsigned int playerAmount;      // Cantidad de jugadores
    playerState players[9];         // Lista de jugadores
    bool isGameOver;                // Indica si el juego se ha terminado
    int boardStart[];               // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} boardGameState;

#endif
