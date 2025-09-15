// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

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
void initializeView(int width, int height, char view_path[], char **environ);
void printStartScreen(syncState *shm_ss, boardGameState *shm_bgs, int delay, int timeout, int seed);
void viewPrint(syncState *shm_ss);
void viewExit();
void viewTerminate();

// masterPlayerManager.c functions
void initializeAllPlayers(int player_count, int board_width, int board_height, char player_paths[][4096], char **environ, int player_paths_fds[MAX_PLAYERS]);
void spawnAllPlayers(boardGameState *shm_bgs, char player_paths[][PATH_MAX]);
bool movePlayer(syncState *shm_ss, boardGameState *shm_bgs, int id, char move);
void exitAllPlayers(syncState *shm_ss, int player_count);
void terminateAllPlayers(int player_count);

void setupSigHandler();
void whoWon(boardGameState *shm_bgs);

sig_atomic_t termination_requested = 0;

// TODO: TEMPORAL!!
boardGameState *glob_shm_bgs;
syncState *glob_shm_ss;

void signalHandler(int signum)
{
    /* TODO: NO BORRAR
    if(signum == SIGTERM || signum == SIGINT)
        termination_requested = 1;
    */
    // TODO: TEMPORAL!!
    viewExit();
    exitAllPlayers(glob_shm_ss, glob_shm_bgs->playerAmount);
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
    setupSigHandler();

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
        initializeView(width, height, view_path, environ);
    }

    // --- Player processes init --- //

    int player_paths_fds[player_count];
    initializeAllPlayers(player_count, width, height, player_bin_paths, environ, player_paths_fds);

    // --- Spawn players --- //
    spawnAllPlayers(shm_bgs, player_bin_paths);

    // --- Game Start --- //

    // - Starting screen - //
    if (view_specified)
    {
        printStartScreen(shm_ss, shm_bgs, delay, timeout, seed);
        viewPrint(shm_ss);
    }

    // --- Round Robin scheduling among players --- //

    int currentPlayerIndex = rand() % player_count;
    time_t lastValidMov = time(NULL);
    int blockedPlayers = 0;
    fd_set readfds;

    while (!shm_bgs->isGameOver && !termination_requested)
    {
        // timeout?
        time_t currentTime = time(NULL);
        if (difftime(currentTime, lastValidMov) >= timeout)
        {
            printf("Timeout global alcanzado: %d segundos sin movimientos válidos. Fin del juego.\n", timeout);
            shm_bgs->isGameOver = 1;
            break;
        }
        else if (blockedPlayers >= player_count)
        {
            // All players blocked
            printf("Todos los jugadores se encuentran bloqueados\n");
            shm_bgs->isGameOver = 1;
            break;
        }

        if (sem_post(&shm_ss->player_can_move_sem[currentPlayerIndex]) == -1)
            errExit("Unexpected error: failed to post to player can move semaphore");

        // Waiting player

        FD_ZERO(&readfds);
        FD_SET(player_paths_fds[currentPlayerIndex], &readfds);

        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        int readyAmountOfFD = select(player_paths_fds[currentPlayerIndex] + 1, &readfds, NULL, NULL, &tv);

        if (readyAmountOfFD > 0)
        {
            unsigned char mov;
            ssize_t r = read(player_paths_fds[currentPlayerIndex], &mov, 1);

            if (r <= 0)
            {
                shm_bgs->players[currentPlayerIndex].isBlocked = 1;
                blockedPlayers++;
            }
            else if (movePlayer(shm_ss, shm_bgs, currentPlayerIndex, mov))
            {
                lastValidMov = time(NULL);
            }
        }
        else if (readyAmountOfFD == 0)
        {
            // Go next player
        }
        else
        {
            errExit("Unexpected error: failed to select a player's file descriptor");
        }

        // - Print, notify view process and wait - //
        if (view_specified)
        {
            viewPrint(shm_ss);
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
                // All players are blocked
                break;
            }
        }
    }

    if (view_specified)
    {
        viewPrint(shm_ss);
        viewExit();
    }
    whoWon(shm_bgs);
    exitAllPlayers(shm_ss, player_count);

    // - Shared memory close - //
    closeShmBoardGameState(shm_bgs);
    closeShmSyncState(shm_ss);

    return 0;
}

void setupSigHandler()
{
    // --- Signal handler setup --- //
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // memset(&sa, 0, sizeof(sa));
    if (sigaction(SIGTERM, &sa, NULL) == -1 || sigaction(SIGINT, &sa, NULL) == -1)
        errExit("Unexpected error: failed to setup signal handler");
}

void whoWon(boardGameState *shm_bgs)
{

    int bestScore = 0;
    int numPlayers = shm_bgs->playerAmount;

    // Best score
    for (int i = 0; i < numPlayers; i++)
    {
        if (shm_bgs->players[i].score > bestScore)
        {
            bestScore = shm_bgs->players[i].score;
        }
    }

    int topScorers[numPlayers];
    int topCount = 0;
    for (int i = 0; i < numPlayers; i++)
    {
        if (shm_bgs->players[i].score == bestScore)
        {
            topScorers[topCount++] = i;
        }
    }

    // If its a draw, search for lowest valid movements
    if (topCount > 1)
    {
        int least_valid_moves = shm_bgs->players[topScorers[0]].validMovementRequests;
        for (int i = 1; i < topCount; i++)
        {
            int id = topScorers[i];
            if (shm_bgs->players[id].validMovementRequests < least_valid_moves)
            {
                least_valid_moves = shm_bgs->players[id].validMovementRequests;
            }
        }
        int first_criteria_winners[topCount];
        int first_criteria_winner_count = 0;
        for (int i = 0; i < topCount; i++)
        {
            int id = topScorers[i];
            if (shm_bgs->players[id].validMovementRequests == least_valid_moves)
                first_criteria_winners[first_criteria_winner_count++] = id;
        }

        if (first_criteria_winner_count > 1)
        {
            int bestInvalids = shm_bgs->players[first_criteria_winners[0]].invalidMovementRequests;
            for (int j = 1; j < first_criteria_winner_count; j++)
            {
                int idx = first_criteria_winners[j];
                if (shm_bgs->players[idx].invalidMovementRequests < bestInvalids)
                {
                    bestInvalids = shm_bgs->players[idx].invalidMovementRequests;
                }
            }

            int second_criteria_winners[first_criteria_winner_count];
            int second_criteria_winner_count = 0;
            for (int j = 0; j < first_criteria_winner_count; j++)
            {
                int idx = first_criteria_winners[j];
                if (shm_bgs->players[idx].invalidMovementRequests == bestInvalids)
                {
                    second_criteria_winners[second_criteria_winner_count++] = idx;
                }
            }
            if (second_criteria_winner_count == 1)
            {
                int idx = second_criteria_winners[0];
                printf("Tenemos un empate por puntos %d y en movimientos válidos %d.\nDecidiremos el ganador por quien tiene menos movimientos inválidos:\n", bestScore, least_valid_moves);
                printf("🏆 El \033[4;32mganador\033[0m es el Jugador %d con solo %d movimientos inválidos.\n",
                       idx + 1, bestInvalids);
            }
            else
            {
                printf("🤝 Empate entre %d jugadores: ", second_criteria_winner_count);
                for (int j = 0; j < second_criteria_winner_count; j++)
                {
                    printf("Jugador %d", second_criteria_winners[j] + 1);
                    if (j < second_criteria_winner_count - 1)
                    {
                        printf(", ");
                    }
                }
                printf(". Todos con %d puntos, %d movimientos válidos y %d movimientos inválidos.\n", bestScore, least_valid_moves, bestInvalids);
            }
        }
        else
        {
            int idx = first_criteria_winners[0];
            printf("Tenemos un empate por puntos %d.\nDecidiremos el ganador por quien tiene menos movimientos válidos:\n", bestScore);
            printf("🏆 El \033[4;32mganador\033[0m es el Jugador %d con solo %d movimientos inválidos.\n",
                   idx + 1, least_valid_moves);
        }
    }
    else
    {
        int idx = topScorers[0];
        printf("🏆 El \033[4;32mganador\033[0m es el Jugador %d con %d puntos!\n",
               idx + 1, bestScore);
    }
    printf("\n");
    printf("PLAYER  POINTS  INVALID-MOVES  VALID-MOVEMENTS BLOCKED X   Y\n");
    for (int i = 0; i < shm_bgs->playerAmount; i++)
    {
        printf("%-8s %-12u %-15u %-11u %-4hhu %-3hu %-3hu\n", shm_bgs->players[i].playerName, shm_bgs->players[i].score, shm_bgs->players[i].invalidMovementRequests,
               shm_bgs->players[i].validMovementRequests, shm_bgs->players[i].isBlocked, shm_bgs->players[i].x, shm_bgs->players[i].y);
    }
    printf("\n");

}
