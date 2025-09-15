// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "../include/shmConstants.h"
#include "../include/errorHandling.h"
#include "../include/maxItoaLength.h"

pid_t viewPid;

void initializeView(int width, int height, char view_path[], char **environ)
{
    // - View process creation - //
    viewPid = fork();
    if (viewPid == -1)
        errExit("Unexpected error: failed to create view process");

    // - View process execution - //
    char heightStr[MAX_ITOA_LENGTH], widthStr[MAX_ITOA_LENGTH];
    snprintf(widthStr, sizeof(widthStr), "%d", width);
    snprintf(heightStr, sizeof(heightStr), "%d", height);

    char *viewArgs[] = {view_path, widthStr, heightStr, NULL};

    if (viewPid == 0)
    {
        if (execve(view_path, viewArgs, environ) == -1)
            errExit("Unexpected error: failed to execute view binary");
    }
}

void printStartScreen(syncState *shm_ss, boardGameState *shm_bgs, int delay, int timeout, int seed)
{
    printf("\033[2J\033[H");
    fflush(stdout);
    printf(".·:''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''':·.\n\
: :        ________   ___  ___   ________   _____ ______    ________              : :\n\
: :       |\\   ____\\ |\\  \\|\\  \\ |\\   __  \\ |\\   _ \\  _   \\ |\\   __  \\             : :\n\
: :       \\ \\  \\___| \\ \\  \\\\\\  \\\\ \\  \\|\\  \\\\ \\  \\\\\\__\\ \\  \\\\ \\  \\|\\  \\            : :\n\
: :        \\ \\  \\     \\ \\   __  \\\\ \\  \\\\\\  \\\\ \\  \\\\|__| \\  \\\\ \\   ____\\           : :\n\
: :         \\ \\  \\____ \\ \\  \\ \\  \\\\ \\  \\\\\\  \\\\ \\  \\    \\ \\  \\\\ \\  \\___|           : :\n\
: :          \\ \\_______\\\\ \\__\\ \\__\\\\ \\_______\\\\ \\__\\    \\ \\__\\\\ \\__\\              : :\n\
: :           \\|_______| \\|__|\\|__| \\|_______| \\|__|     \\|__| \\|__|              : :\n\
: :   ________   ___  ___   ________   _____ ______    ________   ________        : :\n\
: :  |\\   ____\\ |\\  \\|\\  \\ |\\   __  \\ |\\   _ \\  _   \\ |\\   __  \\ |\\   ____\\       : :\n\
: :  \\ \\  \\___| \\ \\  \\\\\\  \\\\ \\  \\|\\  \\\\ \\  \\\\\\__\\ \\  \\\\ \\  \\|\\  \\\\ \\  \\___|_      : :\n\
: :   \\ \\  \\     \\ \\   __  \\\\ \\   __  \\\\ \\  \\\\|__| \\  \\\\ \\   ____\\\\ \\_____  \\     : :\n\
: :    \\ \\  \\____ \\ \\  \\ \\  \\\\ \\  \\ \\  \\\\ \\  \\    \\ \\  \\\\ \\  \\___| \\|____|\\  \\    : :\n\
: :     \\ \\_______\\\\ \\__\\ \\__\\\\ \\__\\ \\__\\\\ \\__\\    \\ \\__\\\\ \\__\\      ____\\_\\  \\   : :\n\
: :      \\|_______| \\|__|\\|__| \\|__|\\|__| \\|__|     \\|__| \\|__|     |\\_________\\  : :\n\
: :                                                                 \\|_________|  : :\n\
'·:...............................................................................:·'\n");
    printf("================================== GAME PARAMETERS ==================================\n");
    printf("\tBoard width: %d\t\tBoard height: %d\t\tSeed: %d\n\
    \tDelay: %d\t\tTimeout: %d\t\t\tPlayer count: %d\n",
           shm_bgs->boardWidth, shm_bgs->boardHeight, seed, delay, timeout, (int)shm_bgs->playerAmount);
    printf("==================================== THE PLAYERS ====================================\n");
    for (int i = 0; i < shm_bgs->playerAmount; i++)
    {
        printf("\t\t\tPlayer %d: %s \t Process %d\n", i, shm_bgs->players[i].playerName, shm_bgs->players[i].processID);
    }
    printf("==================================== STARTING... ====================================\n");
    sleep(3);
    printf("\033[2J\033[H");
    fflush(stdout);
}

void viewPrint(syncState *shm_ss)
{
    if (sem_post(&shm_ss->view_print_pending_sem) == -1)
        errExit("Unexpected error: failed to post to print pending semaphore");

    if (sem_wait(&shm_ss->view_print_done_sem) == -1)
        errExit("Unexpected error: failed to wait for print done semaphore");
}

void viewExit()
{
    // View process should exit by itself, so master shouldn't terminate it.
    int status;
    if (waitpid(viewPid, &status, 0) == -1)
        errExit("Unexpected error: failed to wait for view process to end");
    if (WIFSIGNALED(status))
        printf("View process exited with code (%d)", WTERMSIG(status));
}

void viewTerminate()
{
    kill(viewPid, SIGTERM);
    if (waitpid(viewPid, NULL, 0) == -1)
        errExit("Unexpected error: failed to wait for view process to end");
}