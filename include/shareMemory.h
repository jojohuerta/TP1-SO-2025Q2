#ifndef SHARE_MEMORY_H
#define SHARE_MEMORY_H

#include "shmConstants.h" 

void initRandom();


//Crea y mapea el estado del tablero del juego en memoria compartida y retorna un puntero a la estructura mapeada.
boardGameState* createShmBoardGameState();

//Desvincula y elimina el estado del tablero del juego de la memoria compartida.
void closeShmBoardGameState(boardGameState * shmp);

//Crea y mapea el estado de sincronización del juego en memoria compartida y retorna un puntero a la estructura mapeada.
syncState* createShmSyncState(void);

//Desvincula y elimina la memoria compartida de la estructura de sincronizacion.
void closeShmSyncState(syncState * shmp);

#endif // SHARE_MEMORY_H
