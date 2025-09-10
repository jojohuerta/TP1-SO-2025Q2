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

#include <errno.h>
#include <limits.h>

#include "include/shmConstants.h"
#include "include/shareMemory.h"
#include "include/utilities.h"

//Defaults
#define DEF_WIDTH 10
#define DEF_HEIGHT 10
#define DEF_DELAY_MS 200
#define DEF_TIMEOUT_S 10
#define DEF_SEED (int)time(NULL)
#define DEF_VIEW_PATH ""

//Limits
#define MIN_WIDTH DEF_WIDTH
#define MIN_HEIGHT DEF_HEIGHT
#define MIN_PLAYERS 1
#define MAX_PLAYERS 9

extern char **environ;
extern int optind;
extern char *optarg;

int main(int argc, char *argv[]){

    //Parameter validation
    int width = DEF_WIDTH;
    int height = DEF_HEIGHT;
    int delay = DEF_DELAY_MS;
    int timeout = DEF_TIMEOUT_S;
    int seed = DEF_SEED;
    //char view_path[PATH_MAX] = "";
    char *view_path = NULL;
    char *players[MAX_PLAYERS];
    int player_count = 0;

    char *end;
    errno = 0;
    int opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p")) != -1) {
        switch (opt) {
            case 'w':
                width = strtol(optarg, &end, 10);
                if (width < MIN_WIDTH || errno != 0 || *end != '\0'){
                    char msg[STR_ERR_LENGTH];
                    sprintf(msg, "Illegal param: width must a number, at least %d", MIN_WIDTH);
                    errExit(msg);
                }
                break;
            case 'h':
                height = strtol(optarg, &end, 10);
                if (height < MIN_HEIGHT || errno !=0 || *end != '\0'){
                    char msg[STR_ERR_LENGTH];
                    sprintf(msg, "Illegal param: height must a number, at least %d", MIN_HEIGHT);
                    errExit(msg);
                }
                break;
            case 'd':
                delay = strtol(optarg, &end, 10);
                if(errno != 0) {
                    char msg[STR_ERR_LENGTH];
                    sprintf(msg, "Illegal param: delay must be a number");
                    errExit(msg);
                }
                break;
            case 't':
                timeout = strtol(optarg, &end, 10);
                if(errno != 0) {
                    char msg[STR_ERR_LENGTH];
                    sprintf(msg, "Illegal param: timeout must be a number");
                    errExit(msg);
                }
                break;
            case 's':
                seed = strtol(optarg, &end, 10);
                if(errno != 0) {
                    char msg[STR_ERR_LENGTH];
                    sprintf(msg, "Illegal param: seed must be a number");
                    errExit(msg);
                }
                break;
            case 'v':
                view_path = strdup(optarg);
                //sprintf(view_path, "%s", optarg);              //strdup duplica el string apuntado por el parametro y retorna un puntero a ese nuevo string. Asigna memoria, debe liberarse con free (TODO). Cambié por sprintf para no usar memoria al cuete
                if(view_path == NULL || view_path == "") {
                    char msg[STR_ERR_LENGTH];
                    sprintf(msg, "Illegal param: view path is empty or does not exist");
                    errExit(msg);
                }
                break;
            case 'p':
                // Aquí recogemos manualmente los binarios de jugadores
                while (optind < argc && argv[optind][0] != '-') {
                    if (player_count >= MAX_PLAYERS) {
                        char msg[STR_ERR_LENGTH];
                        sprintf(msg, "Illegal param: a maximum of %d players is supported", MAX_PLAYERS);
                        errExit(msg);
                    }
                    players[player_count++] = strdup(argv[optind++]);       //TODO: free de estos paths
                }
                if (player_count < MIN_PLAYERS) {
                    char msg[STR_ERR_LENGTH];
                    sprintf(msg, "Illegal param: a minimum of %d player paths must be specified", MIN_PLAYERS);
                    errExit(msg);
                }
                break;
            default:
                char msg[STR_ERR_LENGTH];
                sprintf(msg, "Illegal params. Usage: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] [-v view] -p player1 [player2 ...]", argv[0]);
                errExit(msg);
        }
    }

    //Memorias
    boardGameState * shm_bgs;
    syncState * shm_ss;

    shm_bgs = createShmBoardGameState(width, height, player_count, seed);
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
   char * viewArgs[] = {view_path, widthStr, heightStr, NULL};

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

    //Loading of players
    initializeAllPlayers(shm_bgs, player_count, playerPids);

    fd_set readfds;
    int maxfd = -1;

    if (view_path != NULL) {
        if (sem_post(&shm_ss->A) == -1)
            errExit("sem_post A inicial");

        if (sem_wait(&shm_ss->B) == -1)
            errExit("sem_wait B inicial");
    }

    // Para round robin 
    int turn = 0;
    int currentPlayerIndex = 0;
    time_t lastValidMov = time(NULL);
    int blockedPlayers = 0;

    while (1){
        turn++;

        //Veamos si hay timeout
        time_t currentTime = time(NULL);
        if (difftime(currentTime, lastValidMov) >= timeout) {
            printf("Timeout global alcanzado: %d segundos sin movimientos válidos. Fin del juego.\n", timeout);
            shm_bgs->isGameOver = 1;
            break; 
        }

        //Chequeo si todos los jugadores estan bloqueados
        if(blockedPlayers >= player_count){
            printf("Todos los jugadores se encuentran bloqueados\n");
            shm_bgs->isGameOver = 1;
            break; 
        }

        //Nos encargamos de el player que le corresponde el turno
        if (sem_post(&shm_ss->playerSem[currentPlayerIndex]) == -1) {
            errExit("sem_post player_turn");
        }

        //Esperar al jugador a que de su respuesta (al que le corresponde el turno)

        //Se limpia el readfds y se asigna al set el que se debe escuchar
        FD_ZERO(&readfds);
        FD_SET(playerFds[currentPlayerIndex], &readfds);

        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        int readyAmountOfFD = select(playerFds[currentPlayerIndex] + 1, &readfds, NULL, NULL, &tv);

        if (readyAmountOfFD > 0) {
            unsigned char mov;
            ssize_t r = read(playerFds[currentPlayerIndex], &mov, 1);
        
            if (r <= 0) {
            // El jugador se bloqueo (pipe cerrado o error)
                shm_bgs->players[currentPlayerIndex].isBlocked = 1;
                blockedPlayers++;
            } else {
                // Se recibiO un movimiento
                printf("Turno %d del master. Movimiento recibido por parte del jugador %d: %d\n", turn+1, currentPlayerIndex, mov);

                // Lock para modificar el estado compartido. Zona critica
                if (sem_wait(&shm_ss->mutex) == -1)
                    errExit("sem_wait mutex");

                if (sem_wait(&shm_ss->writer) == -1)
                    errExit("sem_wait writer");
                if (sem_post(&shm_ss->writer) == -1)
                    errExit("sem_post writer");

                // Validar y ejecutar movimiento
                int movWasValid = interpretMovement(mov, shm_bgs, currentPlayerIndex);
                if (movWasValid) {
                    lastValidMov = time(NULL);
                }
                if (sem_post(&shm_ss->mutex) == -1)
                    errExit("sem_post mutex");
            }
        } else if (readyAmountOfFD == 0) {
        // No se recibió movimiento dentro del timeout de select.
        // El máster simplemente continúa con el siguiente jugador.
        } else {
            errExit("select");
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

        //usleep esta en microsegundos
        usleep(delay * 1000);  

        //Avanzamos con el Round-Robin. Salteamos a los que estan bloqueados
        currentPlayerIndex = (currentPlayerIndex + 1) % player_count;
        int aux = currentPlayerIndex;
        while (shm_bgs->players[currentPlayerIndex].isBlocked) {
            currentPlayerIndex = (currentPlayerIndex + 1) % player_count;
            if (currentPlayerIndex == aux){
                //Todos los jugadores estan bloqueados
                break;
            }
        }
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
