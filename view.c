#include <stdio.h>
#include <stdlib.h>
#include "include/shmConstants.h"

typedef char bool;


void draw(boardGameState* bgs);

int main(int argc, char* argv[]){
    boardGameState* bgs = calloc(1, sizeof(boardGameState));
    bgs->boardWidth = atoi(argv[1]);
    bgs->boardHeight = atoi(argv[2]);
    draw(bgs);
    return 0;
}

void draw(boardGameState* bgs){
    if (bgs->isGameOver){
        return;
    }
    for (int i = 0; i < bgs->boardWidth; i++){
        for (int j = 0; j < bgs->boardHeight; j++){
            printf ("#");
        }
        printf("\n");
    }
    return;
}