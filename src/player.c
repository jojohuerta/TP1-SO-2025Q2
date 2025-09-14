// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/signal.h>

#include "../include/shmConstants.h"
#include "../include/errorHandling.h"

void setup_sig_handler();

boardGameState *open_shm_bgs(int board_game_state_size);
syncState *open_shm_ss();
void unmapShm(boardGameState *shm_bgs, syncState *shm_ss, int board_game_state_size);

int get_id(boardGameState *shm_bgs);

// playerMovement.c functions
unsigned char playerMovAnalysis(int localBoardState[], unsigned short width, unsigned short height, int playerID, unsigned short playerX, unsigned short playerY);

sig_atomic_t termination_requested = 0;

void signal_handler(int signum)
{
    if (signum == SIGTERM || signum == SIGINT)
        termination_requested = 1;

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{

    // --- Param validation --- //
    if (argc != 3)
        errExit("Unexpected error: illegal params for player binary");

    int width = strtol(argv[1], NULL, 10);
    int height = strtol(argv[2], NULL, 10);
    int board_game_state_size = sizeof(boardGameState) + sizeof(int) * (width * height);

    // --- shm connection  --- //
    boardGameState *shm_bgs = open_shm_bgs(board_game_state_size);
    syncState *shm_ss = open_shm_ss();

    // --- Signal handler setup --- //
    setup_sig_handler();

    // --- Initialize --- //
    int playerID = get_id(shm_bgs);

    if (playerID == -1)
        errExit("Unexpected error: player process not found");

    // --- Play --- //
    int localBoardState[width * height];
    unsigned short currentX, currentY;
    bool is_blocked = 0;

    // Lightswitch
    while (!shm_bgs->isGameOver && !termination_requested && !is_blocked)
    {
        if (sem_wait(&shm_ss->player_can_move_sem[playerID]) == -1)
            errExit("Unexpected error: failed to wait for player can move semaphore");

        if (sem_wait(&shm_ss->game_state_starvation_mutex) == -1)
            errExit("Unexpected error: failed to wait for game state starvation semaphore");


        if (sem_wait(&shm_ss->reader_count_mutex) == -1)
            errExit("Unexpected error: failed to wait for readers count semaphore");

        if (shm_ss->reader_count++ == 0)
        {
            if (sem_wait(&shm_ss->game_state_mutex) == -1)
                errExit("Unexpected error: failed to wait for game state semaphore");
        }

        if (sem_post(&shm_ss->reader_count_mutex) == -1)
            errExit("Unexpected error: failed to post to readers count semaphore");

        memcpy(localBoardState, shm_bgs->boardStart, sizeof(int) * width * height);
        currentX = shm_bgs->players[playerID].x;
        currentY = shm_bgs->players[playerID].y;

        if (sem_wait(&shm_ss->reader_count_mutex) == -1)
            errExit("Unexpected error: failed to wait for readers count semaphore");
        
        if (shm_ss->reader_count-- == 1)
        {
            if (sem_post(&shm_ss->game_state_mutex) == -1)
                errExit("Unexpected error: failed to post to game state semaphore");
        }

        if (sem_post(&shm_ss->reader_count_mutex) == -1)
            errExit("Unexpected error: failed to post to readers count semaphore");

        if (sem_post(&shm_ss->game_state_starvation_mutex) == -1)
            errExit("Unexpected error: failed to post to game state starvation semaphore");

        // --- Player now evaluate which movement to make and sends it to the master --- //

        unsigned char nextMov = playerMovAnalysis(localBoardState, (unsigned short)width, (unsigned short)height, playerID, currentX, currentY);

        if (nextMov < 8)
        {
            if (write(1, &nextMov, 1) == -1)
                errExit("Unexpected error: failed to write move in player pipe");
        }
        else
        {
            is_blocked = 1;
        }
    }

    close(1);
    unmapShm(shm_bgs, shm_ss, board_game_state_size);

    exit(EXIT_SUCCESS);
}

boardGameState *open_shm_bgs(int board_game_state_size)
{
    int fd_bgs = shm_open(GAME_STATE_PATH, O_RDONLY, 0);
    if (fd_bgs == -1)
        errExit("Unexpected error: failed to open game state shared memory");

    boardGameState *shm_bgs = mmap(NULL, board_game_state_size, PROT_READ, MAP_SHARED, fd_bgs, 0);
    if (shm_bgs == MAP_FAILED)
        errExit("Unexpected error: failed to map game state shared memory");

    close(fd_bgs);
    return shm_bgs;
}

syncState *open_shm_ss()
{
    int fd_ss = shm_open(SYNC_STATE_PATH, O_RDWR, 0);
    if (fd_ss == -1)
        errExit("Unexpected error: failed to open sync state shared memory");

    syncState *shm_ss = mmap(NULL, SYNC_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ss, 0);
    if (shm_ss == MAP_FAILED)
        errExit("Unexpected error: failed to map sync state shared memory");

    close(fd_ss);
    return shm_ss;
}

int get_id(boardGameState *shm_bgs)
{
    for (int i = 0; i < shm_bgs->playerAmount; i++)
    {
        if (shm_bgs->players[i].processID == getpid())
        {
            return i;
        }
    }
    return -1;
}

void setup_sig_handler()
{
    // --- Signal handler setup --- //
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) == -1 || sigaction(SIGINT, &sa, NULL) == -1)
        errExit("Unexpected error: failed to setup signal handler");
}

void unmapShm(boardGameState *shm_bgs, syncState *shm_ss, int board_game_state_size)
{
    // --- Unmap shms --- //
    if (munmap(shm_bgs, board_game_state_size) == -1)
    {
        errExit("Unexpected error: failed to unmap game state shared memory");
    }

    if (munmap(shm_ss, SYNC_STATE_SIZE) == -1)
    {
        errExit("Unexpected error: failed to unmap sync state shared memory");
    }
}