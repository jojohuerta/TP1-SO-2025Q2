// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

// #include <stdio.h>
// #include <time.h>
// #include <stddef.h>
// #include <stdlib.h>
// #include <string.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "../include/shmConstants.h"
#include "../include/errorHandling.h"
#include "../include/playerMovement.h"

// playerMovement.c functions
unsigned char playerMovAnalysis(int localBoardState[], unsigned short width, unsigned short height, int playerID, unsigned short playerX, unsigned short playerY);

int main(int argc, char *argv[])
{

    // --- Param validation --- //
    if (argc != 3)
        errExit("Uncaught error: illegal params for player binary");

    int width = strtol(argv[1], NULL, 10);
    int height = strtol(argv[2], NULL, 10);
    int boardGameStateSize = sizeof(boardGameState) + sizeof(int) * (width * height);

    // --- shm connection  --- //
    int fd_bgs, fd_ss;
    boardGameState *shm_bgs;
    syncState *shm_ss;

    // - Game state shm - //
    fd_bgs = shm_open(GAME_STATE_PATH, O_RDONLY, 0);
    if (fd_bgs == -1)
        errExit("Uncaught error: failed to open game state shared memory");

    shm_bgs = mmap(NULL, boardGameStateSize, PROT_READ, MAP_SHARED, fd_bgs, 0);
    if (shm_bgs == MAP_FAILED)
        errExit("Uncaught error: failed to map game state shared memory");

    // - Sync state shm - //
    fd_ss = shm_open(SYNC_STATE_PATH, O_RDWR, 0);
    if (fd_ss == -1)
        errExit("Uncaught error: failed to open sync state shared memory");

    shm_ss = mmap(NULL, SYNC_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ss, 0);
    if (shm_ss == MAP_FAILED)
        errExit("Uncaught error: failed to map sync state shared memory");

    // --- Initialize --- //

    // TODO: revisar toda esta parte
    pid_t my_pid = getpid();
    int playerID = -1;
    for (int i = 0; i < shm_bgs->playerAmount; i++)
    {
        if (shm_bgs->players[i].processID == my_pid)
        {
            playerID = i;
            break;
        }
    }

    if (playerID == -1)
        errExit("playerID not found");

    bool is_blocked;
    bool is_game_over;
    int localBoardState[width*height];

    unsigned short currentX, currentY;

    // --- Play --- //
    // Lightswitch
    while (1)
    {
        // Espera a que el master le de permiso para actuar
        // TODO: revisar lógica de semáforos
        if (sem_wait(&shm_ss->player_can_move_sem[playerID]) == -1)
            errExit("Uncaught error: failed to wait for player can move semaphore");

        if (sem_wait(&shm_ss->game_state_starvation_mutex) == -1)
            errExit("Uncaught error: failed to wait for game state starvation semaphore");
        if (sem_post(&shm_ss->game_state_starvation_mutex) == -1)
            errExit("Uncaught error: failed to post to game state starvation semaphore"); // TODO: qué
        if (sem_wait(&shm_ss->reader_count_mutex) == -1)
            errExit("Uncaught error: failed to wait for readers count semaphore");
        if (shm_ss->reader_count++ == 0)
            if (sem_wait(&shm_ss->game_state_mutex) == -1)
                errExit("Uncaught error: failed to wait for game state semaphore");
        if (sem_post(&shm_ss->reader_count_mutex) == -1)
            errExit("Uncaught error: failed to post to readers count semaphore");

        // Consulta de estado
        memcpy(localBoardState, shm_bgs->boardStart, sizeof(int) * width * height);
        currentX = shm_bgs->players[playerID].x;
        currentY = shm_bgs->players[playerID].y;

        is_game_over = shm_bgs->isGameOver;

        //Consulto el estado a ver si el jugador esta bloqueado
        //Recuerdo que solamente leo y luego ejecuto, porque se deben liberar los semaforos mas adelante
        //Si rompo aca, no los libero y dejo el mutex bloqueado

        if (shm_bgs->players[playerID].isBlocked)
            is_blocked = shm_bgs->players[playerID].isBlocked;

        if (sem_wait(&shm_ss->reader_count_mutex) == -1)
            errExit("Uncaught error: failed to wait for readers count semaphore");
        if (shm_ss->reader_count-- == 1)
            if (sem_post(&shm_ss->game_state_mutex) == -1)
                errExit("Uncaught error: failed to post to game state semaphore");
        if (sem_post(&shm_ss->reader_count_mutex) == -1)
            errExit("Uncaught error: failed to post to readers count semaphore");

        if (is_blocked || is_game_over)
        { // TODO: noooo
            break;
        }

        // Decision y envio del movimiento
        unsigned char nextMov = playerMovAnalysis(localBoardState, (unsigned short)width, (unsigned short)height, playerID, currentX, currentY);

        if (write(1, &nextMov, 1) == -1)
            errExit("write player");
    }

    // --- Unmap shms --- //

    if (munmap(shm_bgs, boardGameStateSize) == -1)
    {
        errExit("Uncaught error: failed to unmap game state shared memory");
    }

    if (munmap(shm_ss, SYNC_STATE_SIZE) == -1)
    {
        errExit("Uncaught error: failed to unmap sync state shared memory");
    }

    exit(EXIT_SUCCESS);
}