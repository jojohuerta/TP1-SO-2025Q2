//#include <ctype.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <stddef.h> 
//#include <string.h>
//#include <time.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/shmConstants.h"
#include "../include/errorHandling.h"

// --- Game state shm --- //
boardGameState * createShmBoardGameState(int boardWidth, int boardHeight, int playerAmount, unsigned int seed){
    int shm_fd;
    char * shm_path = GAME_STATE_PATH;
    boardGameState * shm_addr;
    srand(seed); 

    // - Create game state shm - //
    shm_fd = shm_open(shm_path, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
    if (shm_fd == -1)
        errExit("Uncaught error: failed to create game state shared memory");

    int boardGameStateSize = sizeof(boardGameState) + sizeof(int) * (boardWidth * boardHeight);

    // - Resize shm - //
    if (ftruncate(shm_fd, boardGameStateSize) == -1)
        errExit("Uncaught error: failed to resize game state shared memory");

    // - Map shm to caller process - //
    shm_addr = mmap(NULL, boardGameStateSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_addr == MAP_FAILED)
        errExit("Uncaught error: failed to map game state shared memory");

    // - Close shm's fd after mapping - //
    close(shm_fd);

    // - Initialize shm - //
    shm_addr->boardWidth = boardWidth;
    shm_addr->boardHeight = boardHeight;
    shm_addr->playerAmount = playerAmount;
    shm_addr->isGameOver = 0;
    memset(shm_addr->players, 0, sizeof(shm_addr->players));

    // - Set rewards - //
    for (int i = 0; i < boardHeight * boardWidth; i++) {
        shm_addr->boardStart[i] = (rand() % 8) + 1;
    }

    return shm_addr;
}

void closeShmBoardGameState(boardGameState * shm_addr){

    int boardGameStateSize = sizeof(boardGameState) + sizeof(int) * (shm_addr->boardWidth * shm_addr->boardHeight);

    if (munmap(shm_addr, boardGameStateSize) == -1) {
        errExit("Uncaught error: failed to unmap game state shared memory");
    }

    if (shm_unlink(GAME_STATE_PATH) == -1) {
        errExit("Uncaught error: failed to unlink game state shared memory");
    }
}

syncState* createShmSyncState(){
    int shm_fd;
    char * shm_path = SYNC_STATE_PATH;
    syncState * shm_addr;

    // - Create sync state shm - //
    shm_fd = shm_open(shm_path, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
    if (shm_fd == -1)
        errExit("Uncaught error: failed to create sync state shared memory");

    // - Resize shm - //
    if (ftruncate(shm_fd, SYNC_STATE_SIZE) == -1)
        errExit("Uncaught error: failed to resize sync state shared memory");

    // - Map shm to caller process - //
    shm_addr = mmap(NULL, SYNC_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_addr == MAP_FAILED)
        errExit("Uncaught error: failed to map sync state shared memory");

    // - Close shm's fd after mapping - //
    close(shm_fd);
    
    // - Initialize shm - //
    if (sem_init(&shm_addr->view_print_pending_sem, 1, 0) == -1) errExit("Uncaught error: failed to initialize print pending semaphore");
    if (sem_init(&shm_addr->view_print_done_sem, 1, 0) == -1) errExit("Uncaught error: failed to initialize print done semaphore");
    if (sem_init(&shm_addr->game_state_starvation_mutex, 1, 1) == -1) errExit("Uncaught error: failed to initialize game state starvation semaphore");
    if (sem_init(&shm_addr->game_state_mutex, 1, 1) == -1) errExit("Uncaught error: failed to initialize game state semaphore");
    if (sem_init(&shm_addr->reader_count_mutex, 1, 1) == -1) errExit("Uncaught error: failed to initialize readers count semaphore");
    shm_addr->reader_count = 0;
    
    for (int i = 0; i < 9; i++) {
        if (sem_init(&shm_addr->player_can_move_sem[i], 1, 0) == -1)
            errExit("Uncaught error: failed to initialize player can move semaphores");
    }

    return shm_addr;
}

void closeShmSyncState(syncState * shm_addr){
    
    if (munmap(shm_addr, SYNC_STATE_SIZE) == -1) {
        errExit("Uncaught error: failed to unmap sync state shared memory");
    }

    if (shm_unlink(SYNC_STATE_PATH) == -1) {
        errExit("Uncaught error: failed to unlink sync state shared memory");
    }
}