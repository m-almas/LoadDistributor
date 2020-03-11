#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "dataBlock.h"

void *getSharedMem(key_t ShmKEY);

int getFilledIndex(int *index, int length);

void consumeCamData(struct CamData *camdata);

int main(int argc, char *argv[])
{
    key_t ShmKEY;
    int ShmID;
    pid_t pid;
    struct DataBlock *ShmDataBlock;
    /*to get unigue identifier for sharedMemory*/
    ShmKEY = ftok(".", 23);
    ShmDataBlock = (struct DataBlock *)getSharedMem(ShmKEY);

    for (size_t j = 0; j < 1; j++)
    {
        pid = fork();
        if (pid == 0)
        {
            break;
        }
    }
    if (pid == 0)
    {
        int i;
        int consumedIndex;
        for (;;)
        {
            //something produced

            sem_wait(&ShmDataBlock->produced);
            sem_wait(&ShmDataBlock->cIndexLock);
            //get consumed index and mode it with 10
            consumedIndex = ShmDataBlock->consumedUpTo;
            ShmDataBlock->consumedUpTo = (consumedIndex + 1) % 10;
            sem_post(&ShmDataBlock->cIndexLock);

            //here we get the data and do our stuff
            sleep(1);
            consumeCamData(&(ShmDataBlock->camdata[consumedIndex]));
            fflush(stdout);

            sem_post(&ShmDataBlock->consumed);
        }
    }
    else
    {
        while (pid = waitpid(-1, NULL, 0))
        {
            if (errno == ECHILD)
            {
                break;
            }
        }
    }
}

void *getSharedMem(key_t ShmKEY)
{
    int ShmID;
    struct DataBlock *ShmPTR;

    ShmID = shmget(ShmKEY, sizeof(struct DataBlock), 0666);
    if (ShmID < 0)
    {
        printf("*** shmget error ***\n");
        exit(1);
    }

    ShmPTR = (struct DataBlock *)shmat(ShmID, NULL, 0);
    if ((int)ShmPTR == -1)
    {
        printf("*** shmat error ***\n");
        exit(1);
    }
    return ShmPTR;
}

void consumeCamData(struct CamData *camdata)
{
    printf("----------------------------\n");
    for (size_t i = 0; i < 10; i++)
    {
        for (size_t j = 0; j < 5; j++)
        {
            printf(" %i ", camdata->frames[i].frameData[j]);
            //here we should write it into file
        }
        printf("\n");
    }
    printf("----------------------------\n");
}
