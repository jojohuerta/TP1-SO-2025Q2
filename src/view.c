// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <unistd.h>

#include "../include/shmConstants.h"
#include "../include/errorHandling.h"

typedef enum
{
    PLY1_RED = 31,
    PLY2_BLUE = 34,
    PLY3_GREEN = 32,
    PLY4_YELLOW = 33,
    PLY5_ORANGE = 93,
    PLY6_PURPLE = 95,
    PLY7_CYAN = 36,
    PLY8_MAGENTA = 35,
    PLY9_BLACK = 30
} PlayerColor;

boardGameState *openShmBgs(int board_game_state_size);
syncState *openShmSs();
void setupSigHandler();

void draw(boardGameState *shm_bgs);
void printState(syncState *shm_ss, boardGameState *shm_bgs);
void printGameOverScreen(syncState *shm_ss, boardGameState *shm_bgs);

void unmapShm(boardGameState *shm_bgs, syncState *shm_ss, int board_game_state_size);

sig_atomic_t termination_requested = 0;

void signalHandler(int signum)
{
    if (signum == SIGTERM || signum == SIGINT)
        termination_requested = 1;

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{

    // --- Param validation --- //
    if (argc != 3)
        errExit("Unexpected error: illegal params for view binary");

    int width = strtol(argv[1], NULL, 10);
    int height = strtol(argv[2], NULL, 10);
    int board_game_state_size = sizeof(boardGameState) + sizeof(int) * (width * height);

    // --- shm connection  --- //
    boardGameState *shm_bgs = openShmBgs(board_game_state_size);
    syncState *shm_ss = openShmSs();

    // --- Signal handler setup --- //
    setupSigHandler();

    // --- Print state during game --- //
    while (!shm_bgs->isGameOver && !termination_requested)
    {
        printState(shm_ss, shm_bgs);
    }

    // --- Print game over state --- //
    if (!termination_requested)
        printGameOverScreen(shm_ss, shm_bgs);

    unmapShm(shm_bgs, shm_ss, board_game_state_size);

    return 0;
}

boardGameState *openShmBgs(int board_game_state_size)
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

syncState *openShmSs()
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

void setupSigHandler()
{
    // --- Signal handler setup --- //
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) == -1 || sigaction(SIGINT, &sa, NULL) == -1)
        errExit("Unexpected error: failed to setup signal handler");
}

void draw(boardGameState *shm_bgs)
{
    printf("=====================================================================================\n");
    printf("    P\t\t        PTS   INV-MOV   VAL-MOV    BLOCK   X   Y\n");
    for (int i = 0; i < shm_bgs->playerAmount; i++)
    {
        printf(" %-7s\t\t %-7u %-9u %-9u %-5hhu %-3hu %-3hu\n", shm_bgs->players[i].playerName, shm_bgs->players[i].score, shm_bgs->players[i].invalidMovementRequests,
               shm_bgs->players[i].validMovementRequests, shm_bgs->players[i].isBlocked, shm_bgs->players[i].x, shm_bgs->players[i].y);
    }

    for (int y = 0; y < shm_bgs->boardHeight; y++)
    {
        for (int x = 0; x < shm_bgs->boardWidth; x++)
        {
            printf("|");
            int val = shm_bgs->boardStart[(y * shm_bgs->boardWidth) + x];

            if (val <= 0)
            {
                int idx = -val;
                PlayerColor color = (PlayerColor)(PLY1_RED + idx);

                // Verify is there is a player in this position
                int playerHere = 0;
                for (int i = 0; i < shm_bgs->playerAmount && playerHere == 0; i++)
                {
                    if (shm_bgs->players[i].x == x && shm_bgs->players[i].y == y)
                    {
                        playerHere = 1;
                    }
                }
                if (playerHere)
                {
                    printf("\033[3;4;%dm%d\033[0m", color, idx + 1);
                }
                else
                {
                    printf("\033[%dm%d\033[0m", color, idx + 1);
                }
            }
            else
            {
                printf("%d", val);
            }
        }
        printf("|\n");
    }

    return;
}

void printState(syncState *shm_ss, boardGameState *shm_bgs)
{
    // - Wait until there's something to print - //
    if (!termination_requested)
        if (sem_wait(&shm_ss->view_print_pending_sem) == -1)
            errExit("Unexpected error: failed to wait for print pending semaphore");

    printf("\033[3J\033[H"); // Clear screen and move cursor to top-left
    draw(shm_bgs);

    // - Notify printing done - //
    if (sem_post(&shm_ss->view_print_done_sem) == -1)
        errExit("Unexpected error: failed to post to print done semaphore");
}

void printGameOverScreen(syncState *shm_ss, boardGameState *shm_bgs)
{
    printf("\033[3J\033[H"); // Clear screen and move cursor to top-left
    draw(shm_bgs);
    printf("\n");
    printf("\033[1;31m");
    printf("  #####     #    #     # #######       ######## #       # ####### ######\n");
    printf(" #     #   # #   ##   ## #             #      # #       # #       #     #\n");
    printf(" #        #   #  # # # # #             #      #  #     #  #       #     #\n");
    printf(" #  #### #     # #  #  # #####   ##### #      #  #     #  #####   ######\n");
    printf(" #     # ####### #     # #             #      #   #   #   #       #    #\n");
    printf(" #     # #     # #     # #             #      #    # #    #       #     #\n");
    printf("  #####  #     # #     # #######       ########     #     ####### #      #\n");
    printf("\033[0m");
    
    // Finished, so we notify the master
    if (sem_post(&shm_ss->view_print_done_sem) == -1)
        errExit("Unexpected error: failed to post to print done semaphore");
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
