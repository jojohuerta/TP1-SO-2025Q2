#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "include/shmConstants.h"

void draw(boardGameState* bgs);

//TODO: SHM OPEN Y UNLINK PERO... Y LOS FILE DESCRIPTORS?
int main(int argc, char* argv[]){
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

    //Hay que esperar a que se pueda 
    if (sem_wait(&shm_ss->A) == -1)
        errExit("sem_wait A");

    //Efectivamente, se dibuja
    draw(shm_bgs);

    //Se avisa al master que ya se dibujo
    if (sem_post(&shm_ss->B) == -1)
        errExit("sem_post B");

    munmap(shm_bgs, BOARD_GAME_STATE_SIZE);
    munmap(shm_ss, SYNC_STATE_SIZE);
    return 0;
}

void draw(boardGameState* bgs){
    if (bgs->isGameOver){
        return;
    }
    for (int i = 0; i < bgs->boardWidth; i++){
        for (int j = 0; j < bgs->boardHeight; j++){
            printf ("■");
        }
        printf("\n");
    }
    return;
}