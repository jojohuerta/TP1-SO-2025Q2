#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "include/shmConstants.h"

//TO DO - CORREGIR LAS CONSTANTES DE BOARD GAME STATE
boardGameState * createShmBoardGameState(){
    int fd;
    char * shmpath = GAME_STATE_PATH;
    boardGameState * shmp;

    //Creacion
    fd = shm_open(shmpath, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1)
        errExit("shm_open");

    //expansion
    if (ftruncate(fd, BOARD_GAME_STATE_SIZE) == -1)
        errExit("ftruncate");

    //Mapeo
    shmp = mmap(NULL, BOARD_GAME_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED)
        errExit("mmap");

    close(fd); //TODO, dudoso

    //Inicializacion
    shmp->boardWidth = HEIGHT;
    shmp->boardHeight = HEIGHT;
    shmp->playerAmount = TWO;
    shmp->isGameOver = 0;
    memset(shmp->players, 0, sizeof(shmp->players));
    for (int i = 0; i < HEIGHT * HEIGHT; i++) {
        shmp->boardStart[i] = 0;
    }

    return shmp;
}

void closeShmBoardGameState(boardGameState * shmp){

    if (munmap(shmp, BOARD_GAME_STATE_SIZE) == -1) {
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
    
    /* Initialize semaphores as process-shared, with value 0. */

            //    if (sem_init(&shm_p->sem1, 1, 0) == -1)
            //        errExit("sem_init-sem1");
            //    if (sem_init(&shm_p->sem2, 1, 0) == -1)
            //        errExit("sem_init-sem2");

            //    /* Wait for 'sem1' to be posted by peer before touching
            //       shared memory. */

            //    if (sem_wait(&shmp->sem1) == -1)
            //        errExit("sem_wait");

          /* Convert data in shared memory into upper case. */

            //    for (size_t j = 0; j < shmp->cnt; j++)
            //        shmp->buf[j] = toupper((unsigned char) shmp->buf[j]);

               /* Post 'sem2' to tell the peer that it can now
                  access the modified data in shared memory. */

            //    if (sem_post(&shmp->sem2) == -1)
            //        errExit("sem_post");

               /* Unlink the shared memory object. Even if the peer process
                  is still using the object, this is okay. The object will
                  be removed only after all open references are closed. */    