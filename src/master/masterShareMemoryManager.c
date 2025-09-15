// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <sys/wait.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>
#include <signal.h>

#include "../include/shmConstants.h"
#include "../include/errorHandling.h"

#define BOARD_GAME_STATE_SIZE board_game_state_size

void initialize_game_state(boardGameState *shm_bgs, int boardWidth, int boardHeight, int playerAmount, int seed);
void initialize_sync_state(syncState *shm_ss);

void view_exit();
void exit_all_players(syncState *shm_ss, int player_count);

int board_game_state_size = 0;

// --- Game state shm --- //
boardGameState *createShmBoardGameState(int boardWidth, int boardHeight, int playerAmount, unsigned int seed)
{
    int shm_fd;
    boardGameState *shm_bgs;

    // - Create game state shm - //
    shm_fd = shm_open(GAME_STATE_PATH, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1)
        errExit("Unexpected error: failed to create game state shared memory");

    // - Initialize size - //
    board_game_state_size = sizeof(boardGameState) + sizeof(int) * (boardWidth * boardHeight);

    // - Resize shm - //
    if (ftruncate(shm_fd, BOARD_GAME_STATE_SIZE) == -1)
        errExit("Unexpected error: failed to resize game state shared memory");

    // - Map shm to caller process - //
    shm_bgs = mmap(NULL, BOARD_GAME_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_bgs == MAP_FAILED)
        errExit("Unexpected error: failed to map game state shared memory");

    // - Close shm's fd after mapping - //
    close(shm_fd);

    // - Initialize shm - //
    initialize_game_state(shm_bgs, boardWidth, boardHeight, playerAmount, seed);

    return shm_bgs;
}

void closeShmBoardGameState(boardGameState *shm_bgs)
{

    if (munmap(shm_bgs, BOARD_GAME_STATE_SIZE) == -1)
    {
        errExit("Unexpected error: failed to unmap game state shared memory");
    }

    if (shm_unlink(GAME_STATE_PATH) == -1)
    {
        errExit("Unexpected error: failed to unlink game state shared memory");
    }
}

syncState *createShmSyncState()
{
    int shm_fd;
    syncState *shm_ss;

    // - Create sync state shm - //
    shm_fd = shm_open(SYNC_STATE_PATH, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1)
        errExit("Unexpected error: failed to create sync state shared memory");

    // - Resize shm - //
    if (ftruncate(shm_fd, SYNC_STATE_SIZE) == -1)
        errExit("Unexpected error: failed to resize sync state shared memory");

    // - Map shm to caller process - //
    shm_ss = mmap(NULL, SYNC_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ss == MAP_FAILED)
        errExit("Unexpected error: failed to map sync state shared memory");

    // - Close shm's fd after mapping - //
    close(shm_fd);

    // - Initialize shm - //
    initialize_sync_state(shm_ss);

    // local_shm_ss = shm_addr;
    return shm_ss;
}

void closeShmSyncState(syncState *shm_ss)
{

    // - Destroy all semaphores - //
    if (sem_destroy(&shm_ss->view_print_pending_sem) == -1)
        errExit("Unexpected error: failed to destroy print pending semaphore");
    if (sem_destroy(&shm_ss->view_print_done_sem) == -1)
        errExit("Unexpected error: failed to destroy print done semaphore");
    if (sem_destroy(&shm_ss->game_state_starvation_mutex) == -1)
        errExit("Unexpected error: failed to destroy game state starvation semaphore");
    if (sem_destroy(&shm_ss->game_state_mutex) == -1)
        errExit("Unexpected error: failed to destroy game state semaphore");
    if (sem_destroy(&shm_ss->reader_count_mutex) == -1)
        errExit("Unexpected error: failed to destroy readers count semaphore");

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (sem_destroy(&shm_ss->player_can_move_sem[i]) == -1)
            errExit("Unexpected error: failed to destroy player can move semaphores");
    }

    // - Then unmap - //
    if (munmap(shm_ss, SYNC_STATE_SIZE) == -1)
    {
        errExit("Unexpected error: failed to unmap sync state shared memory");
    }

    if (shm_unlink(SYNC_STATE_PATH) == -1)
    {
        errExit("Unexpected error: failed to unlink sync state shared memory");
    }
}

void initialize_game_state(boardGameState *shm_bgs, int boardWidth, int boardHeight, int playerAmount, int seed)
{
    shm_bgs->boardWidth = boardWidth;
    shm_bgs->boardHeight = boardHeight;
    shm_bgs->playerAmount = playerAmount;
    shm_bgs->isGameOver = 0;
    memset(shm_bgs->players, 0, sizeof(shm_bgs->players));

    // - Set rewards - //
    srand(seed);
    for (int i = 0; i < boardHeight * boardWidth; i++)
    {
        shm_bgs->boardStart[i] = (rand() % 8) + 1;
    }
}

void initialize_sync_state(syncState *shm_ss)
{
    if (sem_init(&shm_ss->view_print_pending_sem, 1, 0) == -1)
        errExit("Unexpected error: failed to initialize print pending semaphore");
    if (sem_init(&shm_ss->view_print_done_sem, 1, 0) == -1)
        errExit("Unexpected error: failed to initialize print done semaphore");
    if (sem_init(&shm_ss->game_state_starvation_mutex, 1, 1) == -1)
        errExit("Unexpected error: failed to initialize game state starvation semaphore");
    if (sem_init(&shm_ss->game_state_mutex, 1, 1) == -1)
        errExit("Unexpected error: failed to initialize game state semaphore");
    if (sem_init(&shm_ss->reader_count_mutex, 1, 1) == -1)
        errExit("Unexpected error: failed to initialize readers count semaphore");
    shm_ss->reader_count = 0;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (sem_init(&shm_ss->player_can_move_sem[i], 1, 0) == -1)
            errExit("Unexpected error: failed to initialize player can move semaphores");
    }
}