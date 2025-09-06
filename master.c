#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include "include/shmConstants.h"
#include "include/shareMemory.h"
#include "include/utilities.h"

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
    
    //INICIALIZACION DE UN PROCESO PLAYER Y CREACION DEL PIPE
    //Pipe anonimo
    int pipefd[2]; //[0] es lectura y [1] es escritura
    int playerStatus;

    if (pipe(pipefd) == -1)
        errExit("pipe player");

    //Creacion del proceso
    pid_t playerPid = fork();
    if (playerPid == -1)
        errExit("fork player");

    char * playerPath = "./player"; 
    char * playerArgs[] = {"./player", heightStr, widthStr, NULL};

    if(playerPid == 0){     //Si el PID = 0 es el hijo
        
        //Cerrado del extremo de lectura del pipe
        close(pipefd[0]); 
        
        //Cerrado del stdout?
        close(1);  

        //Duplicado del file descriptor al mas chico, o sea, 1 (STDOUT)
        //Ahora el extremo del pipe es el STDOUT
        if(dup(pipefd[1]) == -1)
            errExit("dup player");


        //Cerrado del FD del pipe original ya que nos vamos a quedar con STDOUT
        close(pipefd[1]);

        //Ejecucion del jugador
        if(execve(playerPath, playerArgs, environ) == -1)
            errExit("execve player");

    } else {                //El padre

        //se cierra el extremo de escritura
        close(pipefd[1]);

    }
    
    int playerReadFd = pipefd[0];
    
    for (int i = 0; i < 5; i++){

        //NOS ENCARGAMOS DE LOS PLAYERS

        //Recepcion de movimiento por pipes
        unsigned char mov;
        read(playerReadFd, &mov, 1);

        if (sem_wait(&shm_ss->mutex) == -1)
                errExit("sem_wait mutex");
        if (sem_wait(&shm_ss->writer) == -1)
            errExit("sem_wait writer");
        if (sem_post(&shm_ss->writer) == -1)
            errExit("sem_post writer");
        
        //TODO: EJECUTAR MOVIMIENTOS
            //TODO: Validar que sea valido. Por ahora vamos a asumir que si lo es.
            //Aca se ejecuta pero asumiendo que todo sale bien
        interpretMovement(mov, shm_bgs, 0);
        shm_bgs->boardStart[shm_bgs->players[0].x + shm_bgs->players[0].y * shm_bgs->boardHeight]=-1;

        if (sem_post(&shm_ss->mutex) == -1)
            errExit("sem_post mutex");

        //DIBUJARMOS
        
        //Se avisa a la vista que puede imprimir
        if (sem_post(&shm_ss->A) == -1)
            errExit("sem_post A");

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
    
    close(pipefd[0]);
    
    exit(EXIT_SUCCESS);
}
