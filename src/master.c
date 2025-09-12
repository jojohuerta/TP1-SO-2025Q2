#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "../include/shmConstants.h"
#include "../include/errorHandling.h"

// Defaults
#define DEF_WIDTH 10
#define DEF_HEIGHT 10
#define DEF_DELAY_MS 200
#define DEF_TIMEOUT_S 10
#define DEF_SEED (int)time(NULL)
#define DEF_VIEW_PATH ""

// Limits
#define MIN_WIDTH DEF_WIDTH
#define MIN_HEIGHT DEF_HEIGHT
#define MIN_PLAYERS 1
#define MAX_ITOA_LENGTH INT_MAX % 10

// Env vars
extern char **environ;
extern int optind;
extern char *optarg;

// masterPlayerManager.c functions
int interpretMovement(unsigned char mov, boardGameState *shm_bgs, int player);
void initializeAllPlayers(boardGameState *shm_bgs, int playerCount, pid_t *playerPids);

// masterShareMemoryManager.c functions
boardGameState *createShmBoardGameState(int boardWidth, int boardHeight, int playerAmount, unsigned int seed);
void closeShmBoardGameState(boardGameState *shmp);
syncState *createShmSyncState(void);
void closeShmSyncState(syncState *shmp);

int main(int argc, char *argv[])
{

    // --- Parameter validation --- //
    int width = DEF_WIDTH;
    int height = DEF_HEIGHT;
    int delay = DEF_DELAY_MS;
    int timeout = DEF_TIMEOUT_S;
    int seed = DEF_SEED;
    char view_path[PATH_MAX] = DEF_VIEW_PATH;
    char players[MAX_PLAYERS][PATH_MAX];
    int player_count = 0;

    char *str_end;
    errno = 0;
    int opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p")) != -1)
    {
        switch (opt)
        {
        case 'w':
            width = strtol(optarg, &str_end, 10);
            if (width < MIN_WIDTH || errno != 0 || *str_end != '\0')
            {
                char msg[STR_ERR_LENGTH];
                sprintf(msg, "Illegal param: width must be a number, at least %d", MIN_WIDTH);
                errExit(msg);
            }
            break;
        case 'h':
            height = strtol(optarg, &str_end, 10);
            if (height < MIN_HEIGHT || errno != 0 || *str_end != '\0')
            {
                char msg[STR_ERR_LENGTH];
                sprintf(msg, "Illegal param: height must be a number, at least %d", MIN_HEIGHT);
                errExit(msg);
            }
            break;
        case 'd':
            delay = strtol(optarg, &str_end, 10);
            if (errno != 0 || *str_end != '\0')
            {
                char msg[STR_ERR_LENGTH];
                sprintf(msg, "Illegal param: delay must be a number");
                errExit(msg);
            }
            break;
        case 't':
            timeout = strtol(optarg, &str_end, 10);
            if (errno != 0 || *str_end != '\0')
            {
                char msg[STR_ERR_LENGTH];
                sprintf(msg, "Illegal param: timeout must be a number");
                errExit(msg);
            }
            break;
        case 's':
            seed = strtol(optarg, &str_end, 10 || *str_end != '\0');
            if (errno != 0 || *str_end != '\0')
            {
                char msg[STR_ERR_LENGTH];
                sprintf(msg, "Illegal param: seed must be a number");
                errExit(msg);
            }
            break;
        case 'v':
            if (access(optarg, X_OK))
            {
                char msg[STR_ERR_LENGTH];
                sprintf(msg, "Illegal param: view path %s does not exist or you lack necessary permissions", optarg);
                errExit(msg);
            }
            sprintf(view_path, "%s", optarg);
            break;
        case 'p':
            // Aquí recogemos manualmente los binarios de jugadores
            while (optind < argc && argv[optind][0] != '-')
            {
                if (player_count >= MAX_PLAYERS)
                {
                    char msg[STR_ERR_LENGTH];
                    sprintf(msg, "Illegal param: a maximum of %d players is supported", MAX_PLAYERS);
                    errExit(msg);
                }
                if (access(argv[optind], X_OK))
                {
                    char msg[STR_ERR_LENGTH];
                    sprintf(msg, "Illegal param: player path %s does not exist or you lack necessary permissions", argv[optind]);
                    errExit(msg);
                }
                sprintf(players[player_count++], "%s", argv[optind++]);
            }
            if (player_count < MIN_PLAYERS)
            {
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
    // Player check. -p option is mandatory but if no options were specified, the previous cycle is skipped.
    if (player_count < MIN_PLAYERS)
    {
        char msg[STR_ERR_LENGTH];
        sprintf(msg, "Illegal param: a minimum of %d player paths must be specified with option '-p'", MIN_PLAYERS);
        errExit(msg);
    }

    // --- Shared memory init --- //
    // Memorias TODO: revisar
    boardGameState *shm_bgs = createShmBoardGameState(width, height, player_count, seed);
    syncState *shm_ss = createShmSyncState();

    // --- View process init --- //
    int viewStatus;
    pid_t viewPid;
    if (strcmp(view_path, ""))
    {

        // - View process creation - //
        viewPid = fork();
        if (viewPid == -1)
            errExit("Uncaught error: failed to create view process");

        // - View process execution - //
        char heightStr[MAX_ITOA_LENGTH], widthStr[MAX_ITOA_LENGTH];
        snprintf(widthStr, sizeof(widthStr), "%d", width);
        snprintf(heightStr, sizeof(heightStr), "%d", height);

        char *viewArgs[] = {view_path, widthStr, heightStr, NULL};

        if (viewPid == 0)
        {
            if (execve(view_path, viewArgs, environ) == -1)
                errExit("Uncaught error: failed to execute view binary");
        }
    }

    // --- Player processes init --- //
    int pipefd[MAX_PLAYERS][2];
    pid_t playerPids[MAX_PLAYERS];
    int playerFds[MAX_PLAYERS];

    for (int i = 0; i < player_count; i++)
    {

        // - Master-player pipes creation - //
        if (pipe(pipefd[i]) == -1)
            errExit("Uncaught error: failed to create pipe for master-player communication");

        // - Player processeses creation - //
        pid_t pid = fork();
        if (pid == -1)
            errExit("Uncaught error: failed to create player process");

        // - Master-player pipes setup - //
        if (pid == 0)
        {
            // - Player (child) - //

            // Close read end
            close(pipefd[i][0]);

            // Close STDOUT
            close(1);

            // Dupe write end to smallest fd (1), now STDOUT is the pipe's write end
            if (dup(pipefd[i][1]) == -1)
                errExit("Uncaught error: failed to set pipe write end to STDOUT");

            // Close old write end
            close(pipefd[i][1]);

            // - Player processes execution - //
            char widthStr[MAX_ITOA_LENGTH], heightStr[MAX_ITOA_LENGTH];
            snprintf(widthStr, sizeof(widthStr), "%d", width);
            snprintf(heightStr, sizeof(heightStr), "%d", height);

            char *playerArgs[] = {players[i], widthStr, heightStr, NULL};

            if (execve(players[i], playerArgs, environ) == -1)
                errExit("Uncaught error: failed to execute player binary");
        }
        else
        {
            // - Master (parent) - //

            // Close write end
            close(pipefd[i][1]);

            // Save pipes' read ends and players' pids for later use
            playerFds[i] = pipefd[i][0];
            playerPids[i] = pid;
        }
    }

    // --- Player distribution --- //
    initializeAllPlayers(shm_bgs, player_count, playerPids); // TODO: revisar

    fd_set readfds; // TODO: huh?

    // --- Game Start --- //

    // - Starting screen - //
    if (strcmp(view_path, ""))
    {
        if (sem_post(&shm_ss->view_print_pending_sem) == -1)
            errExit("Uncaught error: failed to post to print pending semaphore"); // TODO: nombre semaforos

        if (sem_wait(&shm_ss->view_print_done_sem) == -1)
            errExit("Uncaught error: failed to wait for print done semaphore"); // TODO: nombre semaforos
    }

    // --- Round Robin scheduling among players --- //
    // Para round robin TODO: no deberia ser SCHED_RR?
    int turn = 0;
    int currentPlayerIndex = 0;
    time_t lastValidMov = time(NULL);
    int blockedPlayers = 0;

    while (1)
    {
        turn++;

        // Veamos si hay timeout
        time_t currentTime = time(NULL);
        if (difftime(currentTime, lastValidMov) >= timeout)
        {
            printf("Timeout global alcanzado: %d segundos sin movimientos válidos. Fin del juego.\n", timeout);
            shm_bgs->isGameOver = 1;
            break;
        }

        // Chequeo si todos los jugadores estan bloqueados
        if (blockedPlayers >= player_count)
        {
            printf("Todos los jugadores se encuentran bloqueados\n");
            shm_bgs->isGameOver = 1;
            break;
        }

        // Nos encargamos de el player que le corresponde el turno
        if (sem_post(&shm_ss->player_can_move_sem[currentPlayerIndex]) == -1)
        {
            errExit("Uncaught error: failed to post to player can move semaphore"); // TODO: nombre semaforos
        }

        // Esperar al jugador a que de su respuesta (al que le corresponde el turno)

        // Se limpia el readfds y se asigna al set el que se debe escuchar
        FD_ZERO(&readfds);
        FD_SET(playerFds[currentPlayerIndex], &readfds);

        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        int readyAmountOfFD = select(playerFds[currentPlayerIndex] + 1, &readfds, NULL, NULL, &tv);

        if (readyAmountOfFD > 0)
        {
            unsigned char mov;
            ssize_t r = read(playerFds[currentPlayerIndex], &mov, 1);

            if (r <= 0)
            {
                // El jugador se bloqueo (pipe cerrado o error)
                shm_bgs->players[currentPlayerIndex].isBlocked = 1;
                blockedPlayers++;
            }
            else
            {
                // Se recibiO un movimiento
                printf("Turno %d del master. Movimiento recibido por parte del jugador %d: %d\n", turn + 1, currentPlayerIndex, mov);

                // Lock para modificar el estado compartido. Zona critica
                if (sem_wait(&shm_ss->game_state_mutex) == -1)
                    errExit("Uncaught error: failed to wait for game state semaphore"); // TODO: nombre semaforos

                if (sem_wait(&shm_ss->game_state_starvation_mutex) == -1)
                    errExit("Uncaught error: failed to wait for game state starvation semaphore"); // TODO: nombre semaforos
                if (sem_post(&shm_ss->game_state_starvation_mutex) == -1)
                    errExit("Uncaught error: failed to post to game state starvation semaphore"); // TODO: nombre semaforos

                // Validar y ejecutar movimiento
                int movWasValid = interpretMovement(mov, shm_bgs, currentPlayerIndex);
                if (movWasValid)
                {
                    lastValidMov = time(NULL);
                }
                if (sem_post(&shm_ss->game_state_mutex) == -1)
                    errExit("Uncaught error: failed to post to game state semaphore"); // TODO: nombre semaforos
            }
        }
        else if (readyAmountOfFD == 0)
        {
            // No se recibio ningun movimiento dentro del timeout de select
            // El master continua con el siguiente jugador
        }
        else
        {
            errExit("Uncaught error: failed to select a player's file descriptor");
        }

        // --- Print and delay --- //

        // - Print, notify view process and wait - //
        // DIBUJARMOS
        if (strcmp(view_path, ""))
        {
            // Se avisa a la vista que puede imprimir
            if (sem_post(&shm_ss->view_print_pending_sem) == -1)
                errExit("Uncaught error: failed to post to print pending semaphore"); // TODO: nombre semaforos

            // Esperamos a la vista a que termine de imprimr
            if (sem_wait(&shm_ss->view_print_done_sem) == -1)
                errExit("Uncaught error: failed to wait for print done semaphore"); // TODO: nombre semaforos
        }

        // - Delay - //
        usleep(delay * 1000);

        // Avanzamos con el Round-Robin. Salteamos a los que estan bloqueados
        currentPlayerIndex = (currentPlayerIndex + 1) % player_count;
        int aux = currentPlayerIndex;
        while (shm_bgs->players[currentPlayerIndex].isBlocked)
        {
            currentPlayerIndex = (currentPlayerIndex + 1) % player_count;
            if (currentPlayerIndex == aux)
            {
                // Todos los jugadores estan bloqueados
                break;
            }
        }
    }

    // --- Game over --- //

    shm_bgs->isGameOver = 1;

    // - Print end state - //
    if (strcmp(view_path, ""))
    {
        // Cuando termina el juego se le manda a la vista por ultima vez que imprima
        if (sem_post(&shm_ss->view_print_pending_sem) == -1)
            errExit("Uncaught error: failed to post to print pending semaphore"); // TODO: nombre semaforos

        // Esperamos a que la vista imprima la ultima pantalla
        if (sem_wait(&shm_ss->view_print_done_sem) == -1)
            errExit("Uncaught error: failed to wait for print done semaphore"); // TODO: nombre semaforos

        // - Terminate view - //
        // ESPERAMOS AL HIJO VIEW TODO: revisar porque ya sabe que view_path no es null y solo hace waitpid para view
        if (waitpid(viewPid, &viewStatus, 0) == -1)
            errExit("Uncaught error: failed to terminate view process");
    }

    // - Shared memory close - //
    closeShmBoardGameState(shm_bgs);
    closeShmSyncState(shm_ss);

    // - Pipes' ends close - //
    for (int i = 0; i < player_count; i++)
    {
        close(pipefd[i][0]); // cierre del lado de lectura
        close(pipefd[i][1]);
    }

    exit(EXIT_SUCCESS);
}
