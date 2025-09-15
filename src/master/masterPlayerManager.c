// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "../include/shmConstants.h"
#include "../include/errorHandling.h"
#include "../include/maxItoaLength.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int player_pipes[MAX_PLAYERS][2]; // As master, I will use the read end ([0]).
pid_t player_pids[MAX_PLAYERS];

int initialize_player(int id, int board_width, int board_height, char player_path[PATH_MAX], char **environ)
{
    // - Master-player pipes creation - //
    if (pipe(player_pipes[id]) == -1)
        errExit("Unexpected error: failed to create pipe for master-player communication");
    // - Player processeses creation - //
    pid_t player_pid = fork();
    if (player_pid == -1)
        errExit("Unexpected error: failed to create player process");

    // - Master-player pipes setup - //
    if (player_pid == 0)
    {
        // - Player (child) - //

        // Close read end
        close(player_pipes[id][0]);

        // Close STDOUT
        close(1);

        // Dupe write end to smallest fd (1), now STDOUT is the pipe's write end
        if (dup(player_pipes[id][1]) == -1)
            errExit("Unexpected error: failed to set pipe write end to STDOUT");

        // Close old write end
        close(player_pipes[id][1]);

        // - Player processes execution - //
        char widthStr[MAX_ITOA_LENGTH], heightStr[MAX_ITOA_LENGTH];
        snprintf(widthStr, sizeof(widthStr), "%d", board_width);
        snprintf(heightStr, sizeof(heightStr), "%d", board_height);

        char *player_args[] = {player_path, widthStr, heightStr, NULL};

        if (execve(player_path, player_args, environ) == -1)
            errExit("Unexpected error: failed to execute player binary");
    }
    else
    {
        // - Master (parent) - //

        // Close write end
        close(player_pipes[id][1]);

        // Save players' pids for later use
        // playerFds[i] = pipefd[i][0];
        player_pids[id] = player_pid;
    }
    return player_pipes[id][0];
}

void initialize_all_players(int player_count, int board_width, int board_height, char player_paths[][PATH_MAX], char **environ, int player_pipes_fds[MAX_PLAYERS])
{
    for (int i = 0; i < player_count; i++)
    {
        player_pipes_fds[i] = initialize_player(i, board_width, board_height, player_paths[i], environ);
    }
}

void spawn_player(boardGameState *shm_bgs, int id, char player_path[PATH_MAX])
{

    // Board center
    float cx = (shm_bgs->boardWidth - 1) / 2.0f;
    float cy = (shm_bgs->boardHeight - 1) / 2.0f;

    int x, y;

    if (shm_bgs->playerAmount == 1)
    {
        x = (int)(cx + 0.5f);
        y = (int)(cy + 0.5f);
    }
    else
    {
        // Not to close to the border
        float margin = 2.0f; // padding
        float max_r_x = cx - margin;
        float max_r_y = cy - margin;
        float radius = fminf(max_r_x, max_r_y); // radio max posible

        float angle = (2.0f * M_PI * id) / shm_bgs->playerAmount;

        // Final position
        x = (int)(cx + radius * cosf(angle) + 0.5f);
        y = (int)(cy + radius * sinf(angle) + 0.5f);
    }

    // Init player
    shm_bgs->players[id].x = x;
    shm_bgs->players[id].y = y;
    shm_bgs->players[id].score = 0;
    shm_bgs->players[id].isBlocked = 0;
    shm_bgs->players[id].invalidMovementRequests = 0;
    shm_bgs->players[id].validMovementRequests = 0;
    shm_bgs->players[id].processID = player_pids[id];

    char player_name[MAX_PLAYER_NAME_LENGTH];
    snprintf(player_name, sizeof(player_name), "%s", player_path);
    strcpy(shm_bgs->players[id].playerName, player_name);

    // Update board
    shm_bgs->boardStart[(y * shm_bgs->boardWidth) + x] = (-1) * id;
}

void spawn_all_players(boardGameState *shm_bgs, char player_paths[][PATH_MAX])
{
    for (int i = 0; i < shm_bgs->playerAmount; i++)
    {
        spawn_player(shm_bgs, i, player_paths[i]);
    }
}

bool interpretMovement(unsigned char mov, boardGameState *shm_bgs, int player)
{
    int dx = 0, dy = 0;

    switch (mov)
    {
    case 0:
        dy = -1;
        break; // Arriba
    case 1:
        dy = -1;
        dx = +1;
        break; // Arriba y derecha
    case 2:
        dx = +1;
        break; // Derecha
    case 3:
        dx = +1;
        dy = +1;
        break; // Abajo y derecha
    case 4:
        dy = +1;
        break; // Abajo
    case 5:
        dy = +1;
        dx = -1;
        break; // Abajo e izquierda
    case 6:
        dx = -1;
        break; // izquierda
    case 7:
        dx = -1;
        dy = -1;
        break; // Arriba e izquierda
    default:
        shm_bgs->players[player].invalidMovementRequests++;
        return 0;
    }

    int oldX = shm_bgs->players[player].x;
    int oldY = shm_bgs->players[player].y;
    int newX = oldX + dx;
    int newY = oldY + dy;

    // Validate if it out of bounds
    if (newX < 0 || newX >= shm_bgs->boardWidth || newY < 0 || newY >= shm_bgs->boardHeight)
    {
        shm_bgs->players[player].invalidMovementRequests++;
        return 0;
    }

    int newPosIndex = newX + newY * shm_bgs->boardWidth;

    // Validate if the position is free
    if (shm_bgs->boardStart[newPosIndex] <= 0)
    {
        shm_bgs->players[player].invalidMovementRequests++;
        return 0;
    }

    // Aad score
    shm_bgs->players[player].score += shm_bgs->boardStart[(newY * shm_bgs->boardWidth) + newX];

    // Register new movement
    shm_bgs->players[player].x = newX;
    shm_bgs->players[player].y = newY;

    shm_bgs->boardStart[newPosIndex] = (-1) * player;
    shm_bgs->players[player].validMovementRequests++;
    return 1;
}

bool move_player(syncState *shm_ss, boardGameState *shm_bgs, int id, char move)
{

    bool did_player_move = 0;

    if (sem_wait(&shm_ss->game_state_starvation_mutex) == -1)
        errExit("Unexpected error: failed to wait for game state starvation semaphore");

    if (sem_wait(&shm_ss->game_state_mutex) == -1)
        errExit("Unexpected error: failed to wait for game state semaphore");

    if (interpretMovement(move, shm_bgs, id))
    {
        did_player_move = 1;
    }

    if (sem_post(&shm_ss->game_state_mutex) == -1)
        errExit("Unexpected error: failed to post to game state semaphore");

    if (sem_post(&shm_ss->game_state_starvation_mutex) == -1)
        errExit("Unexpected error: failed to post to game state starvation semaphore");

    return did_player_move;
}

void player_exit(syncState *shm_ss, int id)
{
    close(player_pipes[id][0]);
    // Wake up each player and wait until it exits
    if (sem_post(&shm_ss->player_can_move_sem[id]))
        errExit("Unexpected error: failed to post to player can move semaphore");
    int status;
    if (waitpid(player_pids[id], &status, 0) == -1)
        errExit("Unexpected error: failed to wait for player process to end");
    if (WIFSIGNALED(status))
        printf("Player %d process exited with code (%d)", id, WTERMSIG(status));
}

void exit_all_players(syncState *shm_ss, int player_count)
{
    for (int i = 0; i < player_count; i++)
    {
        player_exit(shm_ss, i);
    }
}

void player_terminate(int id)
{
    close(player_pipes[id][0]);
    kill(player_pids[id], SIGTERM);
    if (waitpid(player_pids[id], NULL, 0) == -1)
        errExit("Unexpected error: failed to wait for view process to end");
}

void terminate_all_players(int player_count)
{
    for (int i = 0; i < player_count; i++)
    {
        player_terminate(i);
    }
}