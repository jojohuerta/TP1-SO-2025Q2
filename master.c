#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "include/shmConstants.h"
#include "include/shareMemory.h"

int main(int argc, char *argv[]){
    boardGameState * shm_bgs;
    syncState * shm_ss;

    shm_bgs = createShmBoardGameState();
    shm_ss = createShmSyncState();

    //Se avisa a la vista que puede imprimir
    if (sem_post(&shm_ss->A) == -1)
        errExit("sem_post A");

    // int fd;
    // char *shmpath;

    //Primero se crean las 2 memorias compartidas
    //Creacion de "/game_state"

    // shmpath = GAME_STATE_PATH;

    /* Create shared memory object and set its size to the size of our structure. */

    // fd = shm_open(shmpath, O_CREAT | O_EXCL | O_RDWR, 0600);
    
    // if (fd == -1)
    // errExit("shm_open");

    // if (ftruncate(fd, GAME_STATE_SIZE) == -1)
    // errExit("ftruncate");

    /* Map the object into the caller's address space. */

    // shmp shm_p = mmap(NULL, sizeof(shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        
    // if (shm_p == MAP_FAILED)
    //     errExit("mmap");

               /* Initialize semaphores as process-shared, with value 0. */

            //    if (sem_init(&shm_p->sem1, 1, 0) == -1)
            //        errExit("sem_init-sem1");
            //    if (sem_init(&shm_p->sem2, 1, 0) == -1)
            //        errExit("sem_init-sem2");

               /* Wait for 'sem1' to be posted by peer before touching
                  shared memory. */

            //    if (sem_wait(&shmp->sem1) == -1)
            //        errExit("sem_wait");

               /* Convert data in shared memory into upper case. */

            //    for (size_t j = 0; j < shmp->cnt; j++)
            //        shmp->buf[j] = toupper((unsigned char) shmp->buf[j]);

               /* Post 'sem2' to tell the peer that it can now
                  access the modified data in shared memory. */

            //    if (sem_post(&shmp->sem2) == -1)
            //        errExit("sem_post");

               /* Unlink the shared memory object. Even if the peer process
                  is still using the object, this is okay. The object will
                  be removed only after all open references are closed. */

    //shm_unlink(shmpath);

    //Esperamos a la vista a que termine de imprimr
    if (sem_wait(&shm_ss->B) == -1)
        errExit("sem_wait B");

    closeShmBoardGameState(shm_bgs);
    closeShmSyncState(shm_ss);

    exit(EXIT_SUCCESS);
}
