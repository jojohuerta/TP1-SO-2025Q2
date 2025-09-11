#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "include/shmConstants.h"
#include "include/utilities.h"
#include "include/playerUtils.h"

int main(int argc, char* argv[]){
    //Trata de parametros
    if (argc != 3)
        errExit("Argumentos incorrectos para player");

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    int boardGameStateSize = sizeof(boardGameState) + sizeof(int) * (width * height);
    
    //Manejo de memoria compartida
    int fd_bgs, fd_ss;
    boardGameState* shm_bgs;
    syncState * shm_ss;

    //Abrimos y mapeamos la shm del gameboard
    fd_bgs = shm_open(GAME_STATE_PATH, O_RDONLY, 0);
    if (fd_bgs == -1)
        errExit("shm_open boardGameState in player");

    shm_bgs = mmap(NULL, boardGameStateSize, PROT_READ, MAP_SHARED, fd_bgs, 0);
    if (shm_bgs == MAP_FAILED)
        errExit("mmap boardGameState in player");

    //abrimos y mapeamos la shm de sincronizacion
    fd_ss = shm_open(SYNC_STATE_PATH, O_RDWR, 0);
    if (fd_ss == -1)
        errExit("shm_open syncState in player");

    shm_ss = mmap(NULL, SYNC_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ss, 0);
    if (shm_ss == MAP_FAILED)
        errExit("mmap syncState in player");

    pid_t my_pid = getpid();
    int playerID = -1;
    for (int i = 0; i < shm_bgs->playerAmount; i++) {
        if (shm_bgs->players[i].processID == my_pid) {
            playerID = i;
            break;
        }
    }

    if (playerID == -1)
        errExit("playerID not found");

    bool blocked;
    int localBoardState[width*height];
    unsigned short currentX, currentY;
    bool isFirstTurn = 1;

    //Lightswitch
    while(1){
    // Espera a que el master le de permiso para actuar
        if (sem_wait(&shm_ss->playerSem[playerID]) == -1)
            errExit("sem_wait playerSem");

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

        //Consulta de estado
        memcpy(localBoardState, shm_bgs->boardStart, sizeof(int) * width * height);
        if (isFirstTurn){
            currentX = shm_bgs->players[playerID].x;
            currentY = shm_bgs->players[playerID].y;
        }

        //Consulto el estado a ver si el jugador esta bloqueado
        //Recuerdo que solamente leo y luego ejecuto, porque se deben liberar los semaforos mas adelante
        //Si rompo aca, no los libero y dejo el mutex bloqueado
        if (shm_bgs->players[playerID].isBlocked)
            blocked = shm_bgs->players[playerID].isBlocked;

        if (sem_wait(&shm_ss->readersCountMutex) == -1)
            errExit("sem_wait readersCountMutex");
        if (shm_ss->readersCount-- == 1)
            if (sem_post(&shm_ss->mutex) == -1)
                errExit("sem_post mutex");
        if (sem_post(&shm_ss->readersCountMutex) == -1)
            errExit("sem_post readersCountMutex");  
        
        if (blocked){
            break;
        }

        //Decision y envio del movimiento
        unsigned char nextMov = playerMovAnalysis(localBoardState,(unsigned short) width,(unsigned short) height, playerID, currentX, currentY);

        if (write(1, &nextMov, 1) == -1)
            errExit("write player");
    }
    
    munmap(shm_bgs, boardGameStateSize);
    munmap(shm_ss, SYNC_STATE_SIZE);
    exit(EXIT_SUCCESS);
}