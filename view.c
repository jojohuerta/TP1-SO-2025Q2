#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "include/shmConstants.h"
#include "include/view.h"

#include "include/utilities.h"

void draw(boardGameState* bgs);

//TODO: SHM OPEN Y UNLINK PERO... Y LOS FILE DESCRIPTORS?
int main(int argc, char* argv[]){

    // --- Param validation --- //
    if (argc != 3)
        errExit("Uncaught error: illegal params for view binary");

    int width = strtol(argv[1], NULL, 10);
    int height = strtol(argv[2], NULL, 10);
    int boardGameStateSize = sizeof(boardGameState) + sizeof(int) * (width * height);

    // --- shm connection  --- //
    int fd_bgs, fd_ss;
    boardGameState* shm_bgs;
    syncState * shm_ss;

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

    // --- Print state during game --- //
    while (1){
        
        // - Wait until there's something to print - //
        if (sem_wait(&shm_ss->view_print_pending_sem) == -1)
            errExit("Uncaught error: failed to wait for print pending semaphore");

        if (shm_bgs->isGameOver)    //TODO: revisar
            break;

        //system("clear"); //Hay alguna mejor opcion? "cls"?
        printf("____________________\n");
        printf("   P  PTS  INV-MOV\n");
        for(int i=0; i<shm_bgs->playerAmount; i++){
            printf("   %d   %d    %d  \n", i+1, shm_bgs->players[i].score, shm_bgs->players[i].invalidMovementRequests);
        }

        draw(shm_bgs);

        // - Notify printing done - //
        if (sem_post(&shm_ss->view_print_pending_sem) == -1)
            errExit("Uncaught error: failed to post to print done semaphore");
    }

    // --- Print game over state --- //

    //Game over screen:
    draw(shm_bgs);
    printf("\033[1;31m"); 
    printf("  #####     #    #     # #######       ######## #       # ####### ######\n");
    printf(" #     #   # #   ##   ## #             #      # #       # #       #     #\n");
    printf(" #        #   #  # # # # #             #      #  #     #  #       #     #\n");
    printf(" #  #### #     # #  #  # #####   ##### #      #  #     #  #####   ######\n");
    printf(" #     # ####### #     # #             #      #   #   #   #       #    #\n");
    printf(" #     # #     # #     # #             #      #    # #    #       #     #\n");
    printf("  #####  #     # #     # #######       ########     #     ####### #      #\n");
    printf("\033[0m"); 
    printf("PLAYER  POINTS  INVALID-MOVES\n");
    for(int i=0; i<shm_bgs->playerAmount; i++){
        printf("  p%d     %d       %d  \n", i+1, shm_bgs->players[i].score, shm_bgs->players[i].invalidMovementRequests);
    }
    printf("\n");

    //Como terminamos tenemos que avisarle al master que ya dibujamos la ultima screen TODO: tenemos? el master espera que le avisemos??
    if (sem_post(&shm_ss->view_print_pending_sem) == -1)
        errExit("Uncaught error: failed to post to print done semaphore");

    // --- Unmap shms --- //

    if (munmap(shm_bgs, boardGameStateSize) == -1) {
        errExit("Uncaught error: failed to unmap game state shared memory");
    }

    if (munmap(shm_ss, SYNC_STATE_SIZE) == -1) {
        errExit("Uncaught error: failed to unmap sync state shared memory");
    }

    return 0;
}


void draw(boardGameState* bgs){
    if (bgs->isGameOver){   //TODO: revisar.
        return;
    }
    for (int y = 0; y < bgs->boardHeight; y++){
        for (int x = 0; x < bgs->boardWidth; x++){
            printf("|");
            int val = bgs->boardStart[(y * bgs->boardWidth) + x];
            if (val == 0) {
                printf("\033[1;%dm%d\033[0m", PLY1_RED, 1);
            } else if (val == -1) {
                printf("\033[1;%dm%d\033[0m", PLY2_BLUE, 2);
            } else if (val == -2) {
                printf("\033[1;%dm%d\033[0m", PLY3_GREEN, 3);
            } else if (val == -3) {
                printf("\033[1;%dm%d\033[0m", PLY4_YELLOW, 4);
            } else if (val == -4) {
                printf("\033[1;%dm%d\033[0m", PLY5_ORANGE, 5);
            } else if (val == -5) {
                printf("\033[1;%dm%d\033[0m", PLY6_PURPLE, 6);
            } else if (val == -6) {
                printf("\033[1;%dm%d\033[0m", PLY7_CYAN, 7);
            } else if (val == -7) {
                printf("\033[1;%dm%d\033[0m", PLY8_MAGENTA, 8);
            } else if (val == -8) {
                printf("\033[1;%dm%d\033[0m", PLY9_BLACK, 9);
            }
            else {
                printf("%d", val);
            }
        }
        printf("\n");
    }
    return;
}