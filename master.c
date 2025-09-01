#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include "include/shmConstants.h"
#include "include/shareMemory.h"

extern char **environ;

int main(int argc, char *argv[]){
    boardGameState * shm_bgs;
    syncState * shm_ss;

    shm_bgs = createShmBoardGameState();
    shm_ss = createShmSyncState();

    //Trata de parametros
    int height = HEIGHT;
    int width = HEIGHT;

    shm_bgs->boardWidth = width;
    shm_bgs->boardHeight = height;
    shm_bgs->isGameOver = 0;

    //INICIALIZACION DEL PROCESO VIEW
    int viewStatus;
    pid_t viewPid = fork();
    if (viewPid == -1)
        errExit("fork view");

    //EXECVE PARA EL VIEW
    char heightStr[16]; 
    char widthStr[16]; 
    snprintf(widthStr, sizeof(widthStr), "%d", width);
    snprintf(heightStr, sizeof(heightStr), "%d", height);
    
    char * viewPath = "./view"; 
    char * viewArgs[] = {"./view", heightStr, widthStr, NULL};

    if(viewPid == 0){   //Si el PID = 0 es el hijo
        if(execve(viewPath, viewArgs, environ) == -1)
            errExit("execve view");
    }

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

    //ESPERAMOS AL HIJO VIEW
    if(waitpid(viewPid, &viewStatus, 0) == -1)
        errExit("view waitpid");

    closeShmBoardGameState(shm_bgs);
    closeShmSyncState(shm_ss);

    exit(EXIT_SUCCESS);
}
