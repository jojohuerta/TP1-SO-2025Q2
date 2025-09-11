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
/* Define a structure that will be imposed on the shared memory object */           

typedef struct {
    sem_t A; // El máster le indica a la vista que hay cambios por imprimir
    sem_t B; // La vista le indica al máster que terminó de imprimir
    sem_t writer; // Mutex para evitar inanición del máster al acceder al estado //WRITER
    sem_t mutex; // Mutex para el estado del juego                              //MUTEX
    sem_t readersCountMutex; // Mutex para la siguiente variable                            //READERS_COUNT_MUTEX
    unsigned int readersCount; // Cantidad de jugadores leyendo el estado              //READERS_COUNT
    sem_t playerSem[9]; // Le indican a cada jugador que puede enviar 1 movimiento  //CON ESTE SE LE AVISA A CADA JUGADOR QUE PUEDE JUGAR
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


#endif  // include guard
