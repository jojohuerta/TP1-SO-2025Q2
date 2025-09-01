#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "include/shmConstants.h"
#include "include/shareMemory.h"

int main(int argc, char *argv[]){
    boardGameState * shm_bgs;
    syncState * shm_ss;

    shm_bgs = createShmBoardGameState();
    shm_ss = createShmSyncState();

    for (int i = 0; i < 10; i++){
        //Se avisa a la vista que puede imprimir
        if (sem_post(&shm_ss->A) == -1)
            errExit("sem_post A");

        //TO-DO: REMOVER ESTO QUE ES PARA TESTEO
        printf("%d\n", i);

        //Esperamos a la vista a que termine de imprimr
        if (sem_wait(&shm_ss->B) == -1)
            errExit("sem_wait B");

        //TO-DO: IMPLEMENTAR EL TIME-OUT PARA LA IMPRESION POR PARAMETRO.
        sleep(1);
    }

    shm_bgs->isGameOver = 1;

    //Cuando termina el juego se le manda a la vista por ultima vez que imprima
    if (sem_post(&shm_ss->A) == -1)
        errExit("sem_post A final");

    //Esperamos a que la vista imprima la ultima pantalla
    if (sem_wait(&shm_ss->B) == -1)
        errExit("sem_wait B final");

    closeShmBoardGameState(shm_bgs);
    closeShmSyncState(shm_ss);

    exit(EXIT_SUCCESS);
}
