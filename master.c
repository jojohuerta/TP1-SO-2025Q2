#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#include "include/shmConstants.h"
#include "include/shareMemory.h"
#include "include/utilities.h"

#define MAX_PLAYERS 9
extern char **environ;
extern int optind;
extern char *optarg;

void loadPlayer1(boardGameState * shm_bgs){
    shm_bgs->players[0].isBlocked=0;
    shm_bgs->players[0].invalidMovementRequests=0;
    shm_bgs->players[0].validMovementRequests=0;
    shm_bgs->players[0].x=4;
    shm_bgs->players[0].y=4;
    shm_bgs->players[0].score=0;
}

int main(int argc, char *argv[]){
    //Trata de parametros
    int width = 10;
    int height = 10;
    int delay = 200;
    int timeout = 1;
    unsigned int seed = (unsigned int)time(NULL);
    char *view_path = NULL;
    char *players[9];
    int player_count = 0;

     int opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p")) != -1) {
        switch (opt) {
            case 'w':
                width = atoi(optarg);
                if (width < 10)
                    errExit("Error: Dimensiones minimas del tablero: 10x10");
                break;
            case 'h':
                height = atoi(optarg);
                if (height < 10)
                    errExit("Error: Dimensiones minimas del tablero: 10x10");
                break;
            case 'd':
                delay = atoi(optarg);
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case 's':
                seed = (unsigned int)atoi(optarg);
                break;
            case 'v':
                view_path = strdup(optarg);  // strdup reserva memoria. <- No tengo ni idea que es esto, averiguar
                break;
            case 'p':
                // Aquí recogemos manualmente los binarios de jugadores
                while (optind < argc && argv[optind][0] != '-') {
                    if (player_count >= 9)
                        errExit("No se pueden especificar mas de 9 jugadores usando -p");
                    players[player_count++] = strdup(argv[optind++]);
                }
                if (player_count < 1) 
                    errExit("Al menos un jugador debe ser especificado usando -p");
                break;
            default:
                fprintf(stderr, "Argumentos incorrectos. Uso: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] [-v view] -p player1 [player2 ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    //Memorias
    boardGameState * shm_bgs;
    syncState * shm_ss;

    shm_bgs = createShmBoardGameState(width, height, player_count);
    shm_ss = createShmSyncState();

    //INICIALIZACION DEL PROCESO VIEW
    int viewStatus;
    pid_t viewPid;
    if (view_path != NULL){    
    viewPid = fork();
    if (viewPid == -1)
        errExit("fork view");

    //EXECVE PARA EL VIEW
    char heightStr[16]; 
    char widthStr[16]; 
    snprintf(widthStr, sizeof(widthStr), "%d", width);
    snprintf(heightStr, sizeof(heightStr), "%d", height);
    
    //char * viewPath = "./view"; 
   char * viewArgs[] = {view_path, heightStr, widthStr, NULL};

    if(viewPid == 0){   //Si el PID = 0 es el hijo
        if(execve(view_path, viewArgs, environ) == -1)
            errExit("execve view");
    }
    }
    
    //INICIALIZACION DE UN PROCESO PLAYER Y CREACION DEL PIPE
    int pipefd[MAX_PLAYERS][2];  //[0] es lectura y [1] es escritura
    pid_t playerPids[MAX_PLAYERS];
    int playerFds[MAX_PLAYERS];  

    for (int i = 0; i < player_count; i++) {
        if (pipe(pipefd[i]) == -1)
            errExit("pipe creation");

        //Creacion del proceso
        pid_t pid = fork();
        if (pid == -1)
            errExit("fork player");

        if(pid == 0){//Si el PID = 0 es el hijo
            //Cerrado del extremo de lectura del pipe
            close(pipefd[i][0]); 
            
            //Cerrado del STDOUT
            close(1);  

            //Duplicado del file descriptor al mas chico, o sea, 1 (STDOUT)
            //Ahora el extremo del pipe es el STDOUT
            if(dup(pipefd[i][1]) == -1)
                errExit("dup player");

            // Cerramos el original porque ya lo tenemos duplicado (en FD 1)
            close(pipefd[i][1]);

            char widthStr[16], heightStr[16];
            snprintf(widthStr, sizeof(widthStr), "%d", width);
            snprintf(heightStr, sizeof(heightStr), "%d", height);

            char *playerArgs[] = {players[i], widthStr, heightStr, NULL};

            // Launch the actual player binary
            if (execve(players[i], playerArgs, environ) == -1)
                errExit("execve player");

        } else {  // Proceso master

            //Cerramos el extremo de escritura y asignamos
            close(pipefd[i][1]);

            playerFds[i] = pipefd[i][0];
            playerPids[i] = pid;
        }
    }

    // int playerReadFd = pipefd[0];
    // loadPlayer1(shm_bgs);

    //temporary loading of players
    for (int i = 0; i < player_count; i++) {
    shm_bgs->players[i].x = 1 + i;  // solo ejemplo
    shm_bgs->players[i].y = 1 + i;
    shm_bgs->players[i].score = 0;
    shm_bgs->players[i].isBlocked = 0;
    shm_bgs->players[i].invalidMovementRequests = 0;
    shm_bgs->players[i].validMovementRequests = 0;
    shm_bgs->players[i].processID = playerPids[i];
    }
    shm_bgs->playerAmount = player_count;

    fd_set readfds;
    int maxfd = -1;

    for (int turn = 0; turn < 5; turn++){

        //NOS ENCARGAMOS DE LOS PLAYERS
        
        // Señal a todos los jugadores para que actuen
        for (int i = 0; i < player_count; i++) {
            if (sem_post(&shm_ss->playerSem[i]) == -1)
                errExit("sem_post playerSem");
        }

        //Se limpia el readfds 
        FD_ZERO(&readfds);

        //Se busca cual es el mas grande (para usar en el select)
        maxfd = -1;

        for (int i = 0; i < player_count; i++) {
            //Se agrega al set y luego se actualiza el maximo.
            FD_SET(playerFds[i], &readfds);
            if (playerFds[i] > maxfd)
                maxfd = playerFds[i];
        }

        // Esperamos que jugadores manden un movimiento o se llegue al timeout
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        int readyAmountOfFD = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (readyAmountOfFD == -1)
            errExit("select");

        // Procesamos los jugadores que mandaron movimiento
        for (int i = 0; i < player_count; i++) {
        if (FD_ISSET(playerFds[i], &readfds)) {
            unsigned char mov;
            read(playerFds[i], &mov, 1);

            printf("Movimiento recibido por parte del jugador %d: %d\n", i, mov);

            // Lock para modificar el estado compartido. Zona critica
            if (sem_wait(&shm_ss->mutex) == -1)
                errExit("sem_wait mutex");

            if (sem_wait(&shm_ss->writer) == -1)
                errExit("sem_wait writer");
            if (sem_post(&shm_ss->writer) == -1)
                errExit("sem_post writer");

            // Ejecutar movimiento
            interpretMovement(mov, shm_bgs, i);

            if (sem_post(&shm_ss->mutex) == -1)
                errExit("sem_post mutex");
        }
        }

        //DIBUJARMOS
        if (view_path != NULL) {
            //Se avisa a la vista que puede imprimir
            if (sem_post(&shm_ss->A) == -1)
                errExit("sem_post A");

            //Esperamos a la vista a que termine de imprimr
            if (sem_wait(&shm_ss->B) == -1)
                errExit("sem_wait B");
        }

        //TO-DO: IMPLEMENTAR EL TIME-OUT PARA LA IMPRESION POR PARAMETRO.
        sleep(1);

    }

    shm_bgs->isGameOver = 1;

    if (view_path != NULL){
    //Cuando termina el juego se le manda a la vista por ultima vez que imprima
    if (sem_post(&shm_ss->A) == -1)
        errExit("sem_post A final");

    //Esperamos a que la vista imprima la ultima pantalla
    if (sem_wait(&shm_ss->B) == -1)
        errExit("sem_wait B final");

    //ESPERAMOS AL HIJO VIEW
    if (view_path != NULL){
        if(waitpid(viewPid, &viewStatus, 0) == -1)
            errExit("view waitpid");
    }
    }
    

    closeShmBoardGameState(shm_bgs);
    closeShmSyncState(shm_ss);

    for (int i = 0; i < player_count; i++) {
    close(pipefd[i][0]); // cierre del lado de lectura
    // También podés cerrar el de escritura si no lo cerraste ya:
    close(pipefd[i][1]);
}
    
    exit(EXIT_SUCCESS);
}
