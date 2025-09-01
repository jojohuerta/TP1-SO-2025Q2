#ifndef SHMCONSTANTS_H
#define SHMCONSTANTS_H

#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

#define GAME_STATE_PATH "/game_state"
#define GAME_STATE_SIZE sizeof(boardGameState) + sizeof(playerState) * 9

#define SYNC_STATE_PATH "/game_sync"
#define SYNC_STATE_SIZE sizeof(syncState)

typedef char bool;
/* Define a structure that will be imposed on the shared memory object */           

typedef struct {
    sem_t A; // El máster le indica a la vista que hay cambios por imprimir
    sem_t B; // La vista le indica al máster que terminó de imprimir
    sem_t C; // Mutex para evitar inanición del máster al acceder al estado
    sem_t D; // Mutex para el estado del juego
    sem_t E; // Mutex para la siguiente variable
    unsigned int F; // Cantidad de jugadores leyendo el estado
    sem_t G[9]; // Le indican a cada jugador que puede enviar 1 movimiento
} syncState;

typedef struct {
    char playerName[16]; // Nombre del jugador
    unsigned int score; // Puntaje
    unsigned int validMovementRequests; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int invalidMovementRequests; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y; // Coordenadas x e y en el tablero
    pid_t processID; // Identificador de proceso
    bool isBlocked; // Indica si el jugador está bloqueado
} playerState;

typedef struct {
    unsigned short boardWidth; // Ancho del tablero
    unsigned short boardHeight; // Alto del tablero
    unsigned int playerAmount; // Cantidad de jugadores
    playerState players[9]; // Lista de jugadores
    bool isGameOver; // Indica si el juego se ha terminado
    int boardStart[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} boardGameState;
       
//TO BE DEPRECATED:

#define BUF_SIZE 1024   /* Maximum size for exchanged string */

struct shmbuf {
    sem_t  sem1;            /* POSIX unnamed semaphore */
    sem_t  sem2;            /* POSIX unnamed semaphore */
    size_t cnt;             /* Number of bytes used in 'buf' */
    char   buf[BUF_SIZE];   /* Data being transferred */
};

#define HEIGHT 8
#define TWO 2
#define BOARD_GAME_STATE_SIZE sizeof(boardGameState) + sizeof(int) * (HEIGHT * HEIGHT)


#endif  // include guard
