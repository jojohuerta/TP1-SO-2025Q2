#ifndef SHMCONSTANTS_H
#define SHMCONSTANTS_H

#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
           #include <stdlib.h>

           #define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                                   } while (0)

           #define BUF_SIZE 1024   /* Maximum size for exchanged string */


typedef char bool;
           /* Define a structure that will be imposed on the shared
              memory object */

           struct shmbuf {
               sem_t  sem1;            /* POSIX unnamed semaphore */
               sem_t  sem2;            /* POSIX unnamed semaphore */
               size_t cnt;             /* Number of bytes used in 'buf' */
               char   buf[BUF_SIZE];   /* Data being transferred */
           };

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
                //playerGameState players[9]; // Lista de jugadores
                bool isGameOver; // Indica si el juego se ha terminado
                int boardStart[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
            } boardGameState;
       
            #endif  // include guard
