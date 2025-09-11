#include "include/view.h"

void draw(boardGameState* bgs);

//TODO: SHM OPEN Y UNLINK PERO... Y LOS FILE DESCRIPTORS?
int main(int argc, char* argv[]){
    if (argc != 3)
        errExit("Argumentos incorrectos para view");

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    int boardGameStateSize = sizeof(boardGameState) + sizeof(int) * (width * height);

    int fd_bgs, fd_ss;
    boardGameState* shm_bgs;
    syncState * shm_ss;
    
    //Abrimos y mapeamos la shm del gameboard
    fd_bgs = shm_open(GAME_STATE_PATH, O_RDONLY, 0);
    if (fd_bgs == -1)
        errExit("shm_open boardGameState in view.");

    shm_bgs = mmap(NULL, boardGameStateSize, PROT_READ, MAP_SHARED, fd_bgs, 0);
    if (shm_bgs == MAP_FAILED)
        errExit("mmap boardGameState in view.");

    //abrimos y mapeamos la shm de sincronizacion
    fd_ss = shm_open(SYNC_STATE_PATH, O_RDWR, 0);
    if (fd_ss == -1)
        errExit("shm_open syncState in view");

    shm_ss = mmap(NULL, SYNC_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ss, 0);
    if (shm_ss == MAP_FAILED)
        errExit("mmap syncState in view");

    int turno = 0;
    while (1){
        //Hay que esperar a que se pueda 
        if (sem_wait(&shm_ss->A) == -1)
            errExit("sem_wait A");

        if (shm_bgs->isGameOver)
            break;

        //system("clear"); //Hay alguna mejor opcion? "cls"?
        draw(shm_bgs);

        //Se avisa al master que ya se dibujo
        if (sem_post(&shm_ss->B) == -1)
            errExit("sem_post B");
    }

    //Game over screen:
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
    printf("PLAYER  POINTS  INVALID-MOVES\n");
    for(int i=0; i<shm_bgs->playerAmount; i++){
        printf("  p%d     %d       %d  \n", i+1, shm_bgs->players[i].score, shm_bgs->players[i].invalidMovementRequests);
    }
    printf("\n");

    whoWon(shm_bgs);

    //Como terminamos tenemos que avisarle al master que ya dibujamos la ultima screen TO-DO mentira, no se diubja la ultima
    if (sem_post(&shm_ss->B) == -1)
        errExit("sem_post B final");

    munmap(shm_bgs, boardGameStateSize);
    munmap(shm_ss, SYNC_STATE_SIZE);
    return 0;
}


void draw(boardGameState* bgs){

    printf("____________________\n");
    printf("   P  PTS  INV-MOV\n");
    for(int i=0; i< bgs->playerAmount; i++){
        printf("   %d   %d    %d  \n", i+1, bgs->players[i].score, bgs->players[i].invalidMovementRequests);
    }

    for (int y = 0; y < bgs->boardHeight; y++){
        for (int x = 0; x < bgs->boardWidth; x++){
            printf("|");
            int val = bgs->boardStart[(y * bgs->boardWidth) + x];
            if (val <= 0) {
                int idx = -val;
                PlayerColor color = (PlayerColor)(PLY1_RED + idx); 
                printf("\033[1;%dm%d\033[0m", color, idx+1);
            } 
            else {
                printf("%d", val);
            }
        }
        printf("\n");
    }
    return;
}

void whoWon(boardGameState* shm_bgs){

    int bestScore = 0;
    int numPlayers = shm_bgs->playerAmount;

    // Buscar el mayor score

    for (int i = 0; i < numPlayers; i++) {
        if (shm_bgs->players[i].score > bestScore) {
            bestScore = shm_bgs->players[i].score;
        }
    }

    // Juntar a los que tienen ese score
    int topScorers[numPlayers];
    int topCount = 0;
    for (int i = 0; i < numPlayers; i++) {
        if (shm_bgs->players[i].score == bestScore) {
            topScorers[topCount++] = i;
        }
    }

    //Si hay empatados de score, buscar el menor número de inválidos
    if (topCount > 1) {
        int bestInvalids = shm_bgs->players[topScorers[0]].invalidMovementRequests;
        for (int j = 1; j < topCount; j++) {  
            int idx = topScorers[j];
            if (shm_bgs->players[idx].invalidMovementRequests < bestInvalids) {
                bestInvalids = shm_bgs->players[idx].invalidMovementRequests;
            }
        }

        int winners[numPlayers];
        int winnerCount = 0;
        for (int j = 0; j < topCount; j++) {
            int idx = topScorers[j];
            if (shm_bgs->players[idx].invalidMovementRequests == bestInvalids) {
                winners[winnerCount++] = idx;
            }
        }

        //Resultado final
        if (winnerCount == 1) {
            int idx = winners[0];
            printf("Tenemos un empate por puntos %d, asique decidiremos el ganador por quien tiene menos movimeintos invalidos\n", shm_bgs->players[idx].score);
            printf("🏆 El \033[4;32mganador\033[0m es el Jugador %d con solo %d movimientos inválidos.\n",
                idx + 1, shm_bgs->players[idx].invalidMovementRequests);
        } else {
            printf("🤝 Empate entre %d jugadores: ", winnerCount);
            for (int j = 0; j < winnerCount; j++) {
                printf("Jugador %d", winners[j] + 1);
                if (j < winnerCount - 1) {
                    printf(", ");
                }
            }
            printf(". Todos con %d puntos y %d movimientos inválidos.\n", bestScore, bestInvalids);
        }
    } else {
        //Solo un jugador con el mejor score
        int idx = topScorers[0];
        printf("🏆 El \033[4;32mganador\033[0m es el Jugador %d con %d puntos y %d movimientos inválidos.\n",
            idx + 1, shm_bgs->players[idx].score, shm_bgs->players[idx].invalidMovementRequests);
    }
    printf("\n");
}