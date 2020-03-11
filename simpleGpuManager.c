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

void consumeCamData(struct CamData *camdata, int fd);

int main(int argc, char *argv[])
{
    key_t ShmKEY;
    int ShmID;
    pid_t pid;
    sem_t io_lock;

    sem_init(&io_lock, 2, 1);

    struct DataBlock *ShmDataBlock;
    /*to get unigue identifier for sharedMemory*/
    ShmKEY = ftok(".", 23);
    ShmDataBlock = (struct DataBlock *)getSharedMem(ShmKEY);

    for (size_t j = 0; j < 2; j++)
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
        int fd;
        fd = open("gpulogs.txt", O_CREAT | O_APPEND | O_WRONLY, 0644);
        for (;;)
        {
            //something produced

            sem_wait(&ShmDataBlock->produced);
            sem_wait(&ShmDataBlock->cIndexLock);
            //get consumed index and mode it with number of camdata blocks
            consumedIndex = ShmDataBlock->consumedUpTo;
            ShmDataBlock->consumedUpTo = (consumedIndex + 1) % (MAX_CAM_NUMBER * 2);
            sem_post(&ShmDataBlock->cIndexLock);

            //here we get the data and do our stuff
            usleep(200000);
            // printf("consumed\n");
            sem_wait(&ShmDataBlock->cIndexLock);
            consumeCamData(&(ShmDataBlock->camdata[consumedIndex]), fd);
            fflush(stdout);
            sem_post(&ShmDataBlock->cIndexLock);
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

void consumeCamData(struct CamData *camdata, int fd)
{
    dprintf(fd, "----------------------------\n");
    dprintf(fd, "The 0'th frame of camera with id %i\n", camdata->camID);
    for (size_t j = 0; j < 5; j++)
    {

        dprintf(fd, " %i ", camdata->frames[0].frameData[j]);
        //here we should write it into file
    }
    dprintf(fd, "\n");
    dprintf(fd, "----------------------------\n");
}
