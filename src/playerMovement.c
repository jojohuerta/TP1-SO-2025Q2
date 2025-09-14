// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdlib.h>

#include <time.h>
#include <unistd.h>

#include "../include/shmConstants.h"

// Valor del espacio libre en la consideracion
#define SPACE_SCORE_MULTIPLIER 1.5f

typedef struct
{
    int x, y;
} Point;

Point newPoint(int x, int y)
{
    Point p;
    p.x = x;
    p.y = y;
    return p;
}

int countAccessibleCells(int startX, int startY, unsigned short width, unsigned short height, const int localBoardState[])
{
    bool visited[height * width];
    for (int i = 0; i < height * width; i++)
    {
        visited[i] = 0;
    }
    Point queueArray[height * width];

    int dx[] = {0, 1, 1, 1, 0, -1, -1, -1}; // Son las direcciones, en orden. 0 = arriba, 1 = arriba a la derecha, etc.
    int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

    int front = 0, rear = 0;
    queueArray[rear++] = newPoint(startX, startY);
    visited[startY * width + startX] = 1;

    int count = 0;

    while (front < rear)
    {
        Point p = queueArray[front++];
        count++;

        for (int i = 0; i < 8; i++)
        {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];

            if (nx >= 0 && nx < width && ny >= 0 && ny < height)
            {
                int index = ny * width + nx;
                int cellValue = localBoardState[index];

                if (!visited[index] && cellValue >= 1 && cellValue <= 9)
                {
                    visited[index] = 1;
                    queueArray[rear++] = newPoint(nx, ny);
                }
            }
        }
    }
    return count;
}

unsigned char playerMovAnalysis(int localBoardState[], unsigned short width, unsigned short height, unsigned int playerID, unsigned short playerX, unsigned short playerY)
{
    int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

    float bestScore = -1.0f;
    int bestMove = -1;

    for (int i = 0; i < 8; i++)
    {
        int nextX = playerX + dx[i];
        int nextY = playerY + dy[i];

        if (nextX >= 0 && nextX < width && nextY >= 0 && nextY < height)
        {
            int index = nextY * width + nextX;
            int cellValue = localBoardState[index];

            if (cellValue >= 1 && cellValue <= 9)
            { // celda libre
                int tempBoard[width * height];
                for (int j = 0; j < width * height; j++) {
                    tempBoard[j] = localBoardState[j];
                }
                tempBoard[index] = -1;

                int accessibleCells = countAccessibleCells(nextX, nextY, width, height, tempBoard);
                float score = cellValue + SPACE_SCORE_MULTIPLIER * accessibleCells;

                if (score > bestScore)
                {
                    bestScore = score;
                    bestMove = i;
                }
            }
        }
    }

    if (bestMove == -1)
    {
        // El jugador no puede continuar
        return 255;
    }

    return bestMove;
}