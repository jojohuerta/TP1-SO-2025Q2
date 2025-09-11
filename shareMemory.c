#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stddef.h> 
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "include/shmConstants.h"
#include "include/utilities.h"

boardGameState * createShmBoardGameState(int boardWidth, int boardHeight, int playerAmount, unsigned int seed){
    int fd;
    char * shmpath = GAME_STATE_PATH;
    boardGameState * shmp;
    srand(seed); 

    //Creacion
    fd = shm_open(shmpath, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1)
        errExit("shm_open");

    int boardGameStateSize = sizeof(boardGameState) + sizeof(int) * (boardWidth * boardHeight);

    //expansion
    if (ftruncate(fd, boardGameStateSize) == -1)
        errExit("ftruncate");

    //Mapeo
    shmp = mmap(NULL, boardGameStateSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED)
        errExit("mmap");

    close(fd);

    //Inicializacion
    shmp->boardWidth = boardWidth;
    shmp->boardHeight = boardHeight;
    shmp->playerAmount = playerAmount;
    shmp->isGameOver = 0;
    memset(shmp->players, 0, sizeof(shmp->players));
    for (int i = 0; i < boardHeight * boardWidth; i++) {
        shmp->boardStart[i] = (rand() % 8) + 1; 
    }

    return shmp;
}

void closeShmBoardGameState(boardGameState * shmp){

    int boardGameStateSize = sizeof(boardGameState) + sizeof(int) * (shmp->boardWidth * shmp->boardHeight);

    if (munmap(shmp, boardGameStateSize) == -1) {
        errExit("Error unmapping shmBoardGameState");
    }

    if (shm_unlink(GAME_STATE_PATH) == -1) {
        errExit("Error unlinking shmBoardGameState");
    }
}

syncState* createShmSyncState(){
    int fd;
    char * shmpath = SYNC_STATE_PATH;
    syncState * shmp;

    //Creacion
    fd = shm_open(shmpath, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1)
        errExit("shm_open");

    //Expansion
    if (ftruncate(fd, SYNC_STATE_SIZE) == -1)
        errExit("ftruncate");

    //Mapeo
    shmp = mmap(NULL, SYNC_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED)
        errExit("mmap");
    
    //Inicializacion
    if (sem_init(&shmp->A, 1, 0) == -1) errExit("sem_init A");
    if (sem_init(&shmp->B, 1, 0) == -1) errExit("sem_init B");
    if (sem_init(&shmp->writer, 1, 1) == -1) errExit("sem_init writer");
    if (sem_init(&shmp->mutex, 1, 1) == -1) errExit("sem_init mutex");
    if (sem_init(&shmp->readersCountMutex, 1, 1) == -1) errExit("sem_init readersCountMutex");
    shmp->readersCount = 0;
    
   for (int i = 0; i < 9; i++) {
        if (sem_init(&shmp->playerSem[i], 1, 0) == -1)
            errExit("sem_init playerSem[i]");
    }

    return shmp;
}

void closeShmSyncState(syncState * shmp){
    if (munmap(shmp, SYNC_STATE_SIZE) == -1) {
        errExit("Error unmapping shmSyncState");
    }

    if (shm_unlink(SYNC_STATE_PATH) == -1) {
        errExit("Error unlinking shmSyncState");
    }
}