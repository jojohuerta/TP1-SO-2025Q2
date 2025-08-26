#include <stdio.h>
#include <stdlib.h>

typedef char bool;

typedef struct {
unsigned short boardWidth; // Ancho del tablero
unsigned short boardHeight; // Alto del tablero
unsigned int playerAmount; // Cantidad de jugadores
//playerGameState players[9]; // Lista de jugadores
bool isGameOver; // Indica si el juego se ha terminado
int boardStart[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} boardGameState;

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
    
/*
int main(int argc, char* argv[]){
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    printf("%d", width);
    printf("%d", height);
    
    for (int i = 0; i < width; i++){
        for (int j = 0; j < height; j++){
            printf("a");
        }
        printf("\n");
    }      
}
    */