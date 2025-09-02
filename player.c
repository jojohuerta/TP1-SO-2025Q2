#include <stdio.h>
#include <time.h>
#include "include/shmConstants.h"
#include "include/utilities.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

void loadPlayer1(boardGameState * shm_bgs){
    shm_bgs->players[0].isBlocked=0;
    shm_bgs->players[0].invalidMovementRequests=0;
    shm_bgs->players[0].validMovementRequests=0;
    shm_bgs->players[0].x=4;
    shm_bgs->players[0].y=4;
    shm_bgs->players[0].score=0;
}

int main(int argc, char* argv[]){
    //Trata de parametros
    if (argc != 3)
        errExit("Argumentos incorrectos para player");

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    
    initRandom();

    //Manejo de memoria compartida
    int fd_bgs, fd_ss;
    boardGameState* shm_bgs;
    syncState * shm_ss;

    //Abrimos y mapeamos la shm del gameboard
    fd_bgs = shm_open(GAME_STATE_PATH, O_RDWR, 0);
    if (fd_bgs == -1)
        errExit("shm_open boardGameState in view");

    shm_bgs = mmap(NULL, BOARD_GAME_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bgs, 0);
    if (shm_bgs == MAP_FAILED)
        errExit("mmap boardGameState in view");

    //abrimos y mapeamos la shm de sincronizacion
    fd_ss = shm_open(SYNC_STATE_PATH, O_RDWR, 0);
    if (fd_ss == -1)
        errExit("shm_open syncState in view");

    shm_ss = mmap(NULL, SYNC_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ss, 0);
    if (shm_ss == MAP_FAILED)
        errExit("mmap syncState in view");

    loadPlayer1(shm_bgs);
    //LIGHTSWITCH! 
    while(1){
        if (sem_wait(&shm_ss->writer) == -1)
            errExit("sem_wait writer");
        if (sem_post(&shm_ss->writer) == -1)
            errExit("sem_post writer");
        if (sem_wait(&shm_ss->readersCountMutex) == -1)
            errExit("sem_wait readersCountMutex");
        if (shm_ss->readersCount++ == 0)
            if (sem_wait(&shm_ss->mutex) == -1)
                errExit("sem_wait mutex");
        if (sem_post(&shm_ss->readersCountMutex) == -1) 
            errExit("sem_post readersCountMutex");

        //TO DO: CONSULTAR ESTADO

        if (sem_wait(&shm_ss->readersCountMutex) == -1)
            errExit("sem_wait readersCountMutex");
        if (shm_ss->readersCount-- == 1)
            if (sem_post(&shm_ss->mutex) == -1)
                errExit("sem_post mutex");
        if (sem_post(&shm_ss->readersCountMutex) == -1)
            errExit("sem_post readersCountMutex");  
            
        //TO DO: DECIDIR SIGUIENTE MOVIMIENTO
        unsigned char nextMov = movAnalysis();
        //TO DO: ENVIAR MOVIMIENTO
        if (write(1, &nextMov, 1) == -1)
            errExit("write player");
    }
    exit(EXIT_SUCCESS);
}