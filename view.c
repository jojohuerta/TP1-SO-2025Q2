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
    printf("PLAYER  POINTS  INVALID-MOVES  VALID-MOVEMENTS BLOCKED X   Y\n");
    for(int i=0; i<shm_bgs->playerAmount; i++){
    printf("%-8s %-12d %-15d %-11d %-4d %-3d %-3d\n", shm_bgs->players[i].playerName, shm_bgs->players[i].score, shm_bgs->players[i].invalidMovementRequests,
            shm_bgs->players[i].validMovementRequests, shm_bgs->players[i].isBlocked, shm_bgs->players[i].x, shm_bgs->players[i].y);
        }
    printf("\n");

    whoWon(shm_bgs);

    //Como terminamos tenemos que avisarle al master que ya dibujamos la ultima screen
    if (sem_post(&shm_ss->B) == -1)
        errExit("sem_post B final");

    munmap(shm_bgs, boardGameStateSize);
    munmap(shm_ss, SYNC_STATE_SIZE);
    return 0;
}


void draw(boardGameState* bgs){

    printf("==============================================\n");
    printf("    P    PTS   INV-MOV   VAL-MOV   BLOCK   X   Y\n");
    for (int i = 0; i < bgs->playerAmount; i++) {
        printf(" %-7s %-7d %-9d %-9d %-5d %-3d %-3d\n", bgs->players[i].playerName, bgs->players[i].score,bgs->players[i].invalidMovementRequests,
            bgs->players[i].validMovementRequests, bgs->players[i].isBlocked, bgs->players[i].x, bgs->players[i].y
        );
    }

    for (int y = 0; y < bgs->boardHeight; y++) {
        for (int x = 0; x < bgs->boardWidth; x++) {
            printf("|");
            int val = bgs->boardStart[(y * bgs->boardWidth) + x];

            if (val <= 0) {
                int idx = -val;  
                PlayerColor color = (PlayerColor)(PLY1_RED + idx);

                // Verifico si hay un jugador parado en esta casilla
                int playerHere = 0;
                for (int i = 0; i < bgs->playerAmount; i++) {
                    if (bgs->players[i].x == x && bgs->players[i].y == y) {
                        playerHere = 1;
                        break;
                    }
                }
                if (playerHere) {
                    printf("\033[3;4;%dm%d\033[0m", color, idx+1);
                } else {
                    printf("\033[%dm%d\033[0m", color, idx+1);
                }
            } else {
                printf("%d", val);
            }
        }
        printf("|\n");
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
        int idx = topScorers[0];
        printf("🏆 El \033[4;32mganador\033[0m es el Jugador %d con %d puntos y %d movimientos inválidos.\n",
            idx + 1, shm_bgs->players[idx].score, shm_bgs->players[idx].invalidMovementRequests);
    }
    printf("\n");
}