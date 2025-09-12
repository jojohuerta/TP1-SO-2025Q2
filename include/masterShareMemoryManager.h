#ifndef SHARE_MEMORY_H
#define SHARE_MEMORY_H

#include "shmConstants.h" 

//Crea y mapea el estado del tablero del juego en memoria compartida y retorna un puntero a la estructura mapeada.
boardGameState * createShmBoardGameState(int boardWidth, int boardHeight, int playerAmount, unsigned int seed );

//Desvincula y elimina el estado del tablero del juego de la memoria compartida.
void closeShmBoardGameState(boardGameState * shmp);

//Crea y mapea el estado de sincronización del juego en memoria compartida y retorna un puntero a la estructura mapeada.
syncState* createShmSyncState(void);

//Desvincula y elimina la memoria compartida de la estructura de sincronizacion.
void closeShmSyncState(syncState * shmp);

void sigtermHandler(int signum);

void safeStorePipefd(int pipefd[][2]);

void safeStoreViewPid(pid_t viewPid);

void safeStorePlayerPids(pid_t playerPids[], int count);

#endif // SHARE_MEMORY_H
