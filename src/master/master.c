// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/* Included in shmConstants.h
#include <sys/types.h>
#include <semaphore.h>
*/

/* Included in errorHandling.h
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
*/

/* Included in maxItoaLength.h
#include <limits.h>
*/

#include <sys/mman.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "../include/shmConstants.h"
#include "../include/errorHandling.h"
#include "../include/maxItoaLength.h"

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

// Env vars
extern char **environ;
extern int optind;
extern char *optarg;

// masterShareMemoryManager.c functions
boardGameState *createShmBoardGameState(int boardWidth, int boardHeight, int playerAmount, unsigned int seed);
void closeShmBoardGameState(boardGameState *shmp);
syncState *createShmSyncState(void);
void closeShmSyncState(syncState *shmp);

// masterViewManager.c functions
void initialize_view(int width, int height, char view_path[], char **environ);
void print_start_screen(syncState *shm_ss, boardGameState *shm_bgs, int delay, int timeout, int seed);
void view_print(syncState *shm_ss);
void view_exit();
void view_terminate();

// masterPlayerManager.c functions
void initialize_all_players(int player_count, int board_width, int board_height, char player_paths[][4096], char **environ);
void spawn_all_players(boardGameState *shm_bgs, char player_paths[][PATH_MAX]);
int interpretMovement(unsigned char mov, boardGameState *shm_bgs, int player);
void initializeAllPlayers(boardGameState *shm_bgs, int playerCount, pid_t *playerPids, char player_bin_paths[][PATH_MAX]);
bool move_player(syncState *shm_ss, boardGameState *shm_bgs, int id, char move);
void exit_all_players(syncState *shm_ss, int player_count);
void terminate_all_players(int player_count);

void setup_sig_handler();

sig_atomic_t termination_requested = 0;

// TODO: TEMPORAL!!
boardGameState *glob_shm_bgs;
syncState *glob_shm_ss;

void signal_handler(int signum)
{
    /* TODO: NO BORRAR
    if(signum == SIGTERM || signum == SIGINT)
        termination_requested = 1;
    */
    // TODO: TEMPORAL!!
    view_exit();
    exit_all_players(glob_shm_ss, glob_shm_bgs->playerAmount);
    closeShmBoardGameState(glob_shm_bgs);
    closeShmSyncState(glob_shm_ss);

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{

    // --- Parameter validation --- //
    int width = DEF_WIDTH;
    int height = DEF_HEIGHT;
    int delay = DEF_DELAY_MS;
    int timeout = DEF_TIMEOUT_S;
    int seed = DEF_SEED;
    char view_path[PATH_MAX] = DEF_VIEW_PATH;
    int player_count = 0;
    char player_bin_paths[MAX_PLAYERS][PATH_MAX];

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
                snprintf(msg, sizeof(msg), "Illegal param: width must be a number, at least %d", MIN_WIDTH);
                errExit(msg);
            }
            break;
        case 'h':
            height = strtol(optarg, &str_end, 10);
            if (height < MIN_HEIGHT || errno != 0 || *str_end != '\0')
            {
                char msg[STR_ERR_LENGTH];
                snprintf(msg, sizeof(msg), "Illegal param: height must be a number, at least %d", MIN_HEIGHT);
                errExit(msg);
            }
            break;
        case 'd':
            delay = strtol(optarg, &str_end, 10);
            if (errno != 0 || *str_end != '\0')
            {
                char msg[STR_ERR_LENGTH];
                snprintf(msg, sizeof(msg), "Illegal param: delay must be a number");
                errExit(msg);
            }
            break;
        case 't':
            timeout = strtol(optarg, &str_end, 10);
            if (errno != 0 || *str_end != '\0')
            {
                char msg[STR_ERR_LENGTH];
                snprintf(msg, sizeof(msg), "Illegal param: timeout must be a number");
                errExit(msg);
            }
            break;
        case 's':
            seed = strtol(optarg, &str_end, 10);
            if (errno != 0 || *str_end != '\0')
            {
                char msg[STR_ERR_LENGTH];
                snprintf(msg, sizeof(msg), "Illegal param: seed must be a number");
                errExit(msg);
            }
            break;
        case 'v':
            if (access(optarg, X_OK))
            {
                char msg[STR_ERR_LENGTH];
                snprintf(msg, sizeof(msg), "Illegal param: view path %s does not exist or you lack necessary permissions", optarg);
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
                    snprintf(msg, sizeof(msg), "Illegal param: a maximum of %d players is supported", MAX_PLAYERS);
                    errExit(msg);
                }
                if (access(argv[optind], X_OK))
                {
                    char msg[STR_ERR_LENGTH];
                    snprintf(msg, sizeof(msg), "Illegal param: player path %s does not exist or you lack necessary permissions", argv[optind]);
                    errExit(msg);
                }
                if (strlen(argv[optind]) >= PATH_MAX)
                {
                    char msg[STR_ERR_LENGTH];
                    snprintf(msg, sizeof(msg), "Illegal param: player path too long (max %d characters)", PATH_MAX - 1);
                    errExit(msg);
                }
                snprintf(player_bin_paths[player_count++], PATH_MAX, "%s", argv[optind++]);
            }
            break;
        default:
            char msg[STR_ERR_LENGTH];
            snprintf(msg, sizeof(msg), "Illegal params. Usage: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] [-v view] -p player1 [player2 ...]", argv[0]);
            errExit(msg);
        }
    }
    // Player check. -p option is mandatory but if no options were specified, the previous cycle is skipped.
    if (player_count < MIN_PLAYERS)
    {
        char msg[STR_ERR_LENGTH];
        snprintf(msg, sizeof(msg), "Illegal param: a minimum of %d player paths must be specified with option '-p'", MIN_PLAYERS);
        errExit(msg);
    }

    // --- Signal handler setup --- //
    setup_sig_handler();

    // --- Shared memory init --- //
    boardGameState *shm_bgs = createShmBoardGameState(width, height, player_count, seed);
    syncState *shm_ss = createShmSyncState();

    // TODO: TEMPORAL!!!
    glob_shm_bgs = shm_bgs;
    glob_shm_ss = shm_ss;

    // --- View process init --- //
    bool view_specified = strcmp(view_path, "") != 0;
    if (view_specified)
    {
        initialize_view(width, height, view_path, environ);
    }

    // --- Player processes init --- //

    // initialize_all_players(player_count, width, height, player_bin_paths, environ);

    int pipefd[player_count][2];
    pid_t playerPids[player_count];
    int playerFds[player_count];

    for (int i = 0; i < player_count; i++)
    {

        // - Master-player pipes creation - //
        if (pipe(pipefd[i]) == -1)
            errExit("Unexpected error: failed to create pipe for master-player communication");

        // - Player processeses creation - //
        pid_t pid = fork();
        if (pid == -1)
            errExit("Unexpected error: failed to create player process");

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
                errExit("Unexpected error: failed to set pipe write end to STDOUT");

            // Close old write end
            close(pipefd[i][1]);

            // - Player processes execution - //
            char widthStr[MAX_ITOA_LENGTH], heightStr[MAX_ITOA_LENGTH];
            snprintf(widthStr, sizeof(widthStr), "%d", width);
            snprintf(heightStr, sizeof(heightStr), "%d", height);

            char *playerArgs[] = {player_bin_paths[i], widthStr, heightStr, NULL};

            if (execve(player_bin_paths[i], playerArgs, environ) == -1)
                errExit("Unexpected error: failed to execute player binary");
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

    // safeStorePipefd(pipefd);
    // safeStorePlayerPids(playerPids, player_count);

    // --- Spawn players --- //
    initializeAllPlayers(shm_bgs, player_count, playerPids, player_bin_paths); // TODO: revisar
    // spawn_all_players(shm_bgs, player_bin_paths);

    // --- Game Start --- //

    // - Starting screen - //
    if (view_specified)
    {
        print_start_screen(shm_ss, shm_bgs, delay, timeout, seed);
        view_print(shm_ss);
    }

    // --- Round Robin scheduling among players --- //
    // Para round robin TODO: no deberia ser SCHED_RR? REVISAR!
    int turn = 0;
    int currentPlayerIndex = 0;
    time_t lastValidMov = time(NULL);
    int blockedPlayers = 0;
    fd_set readfds;

    while (!shm_bgs->isGameOver && !termination_requested)
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
        else if (blockedPlayers >= player_count)
        {
            // Chequeo si todos los jugadores estan bloqueados
            printf("Todos los jugadores se encuentran bloqueados\n");
            shm_bgs->isGameOver = 1;
            break;
        }

        // Nos encargamos de el player que le corresponde el turno
        if (sem_post(&shm_ss->player_can_move_sem[currentPlayerIndex]) == -1)
            errExit("Unexpected error: failed to post to player can move semaphore"); // TODO: nombre semaforos

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
            else if (move_player(shm_ss, shm_bgs, currentPlayerIndex, mov))
            {
                lastValidMov = time(NULL);
            }
        }
        else if (readyAmountOfFD == 0)
        {
            // No se recibio ningun movimiento dentro del timeout de select
            // El master continua con el siguiente jugador
        }
        else
        {
            errExit("Unexpected error: failed to select a player's file descriptor");
        }

        // --- Print and delay --- //

        // - Print, notify view process and wait - //
        // DIBUJARMOS
        if (view_specified)
        {
            view_print(shm_ss);
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

    // - Print final state - //
    /* TODO: NO BORRAR
    if (view_specified)
    {
        if(termination_requested) {
            view_terminate(shm_ss);
        } else {
            view_print(shm_ss);
            view_exit();
        }
    }

    if(termination_requested)
        terminate_all_players(player_count);
    else
        exit_all_players(shm_ss, player_count);
    */

    // TODO: TEMPORAL!!
    if (view_specified)
    {
        view_print(shm_ss);
        view_exit();
    }
    exit_all_players(shm_ss, player_count);

    // - Shared memory close - //
    closeShmBoardGameState(shm_bgs);
    closeShmSyncState(shm_ss);

    return 0;
}

void setup_sig_handler()
{
    // --- Signal handler setup --- //
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // memset(&sa, 0, sizeof(sa));
    if (sigaction(SIGTERM, &sa, NULL) == -1 || sigaction(SIGINT, &sa, NULL) == -1)
        errExit("Unexpected error: failed to setup signal handler");
}