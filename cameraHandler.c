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
//commandStatus
#define ADD 2
#define REMOVE 3
#define STOP 5
//**

void *getSharedMem();
void produceCamData(struct CamData *camdata);
int main(int argc, char *argv[])
{
    key_t ShmKEY;
    int ShmID;
    int status;
    pid_t pid;
    struct DataBlock *ShmDataBlock;

    /*to get unigue identifier for shared memory*/
    ShmKEY = ftok(".", 23);
    ShmDataBlock = (struct DataBlock *)getSharedMem(ShmKEY);
    //semaphores everything is initialized and up to go

    //initblock
    for (size_t i = 0; i < ShmDataBlock->numberOfActiveCameras; i++)
    {
        pid = fork();

        if (pid == 0)
        {
            break;
        }
    }
    //***
    //controllblock
    if (pid > 0)
    {
        for (;;)
        {
            sem_wait(&ShmDataBlock->commandIndicator);
            if (ShmDataBlock->commandType == REMOVE)
            {
                wait(&status);
            }
            else if (ShmDataBlock->commandType == ADD)
            {
                pid = fork();
            }
            else if (&ShmDataBlock->commandType == STOP)
            {
                //should be called only if there is 0 active processes
                exit(0);
            }
            if (pid == 0)
            {
                break;
            }
        }
    }
    //***
    //do somethingblock
    if (pid == 0)
    {
        int fd;
        fd = open("logs.txt", O_CREAT | O_APPEND | O_WRONLY, 0644);
        struct CameraSocket *chosenCamera;
        for (size_t i = 0; i < 10; i++)
        {
            chosenCamera = &ShmDataBlock->cameraSockets[i];
            sem_wait(&chosenCamera->cStatusLock);
            if (chosenCamera->cameraStatus == ON)
            {
                break;
            }
            sem_post(&chosenCamera->cStatusLock);
        }
        chosenCamera->cameraStatus = BUSY;
        sem_post(&chosenCamera->cStatusLock);
        //camera is chosen for service
        int producedIndex;
        int dataIndex;
        for (;;)
        {
            //to check if camera was shutdown
            sem_wait(&chosenCamera->cStatusLock);
            if (chosenCamera->cameraStatus == OFF)
            {
                sem_post(&chosenCamera->cStatusLock);
                break;
            }
            sem_post(&chosenCamera->cStatusLock);
            //***

            //here we should serve
            sem_wait(&ShmDataBlock->consumed);
            sem_wait(&ShmDataBlock->pIndexLock);

            //get produced index and mode it with 10
            producedIndex = ShmDataBlock->producedUpTo;
            ShmDataBlock->producedUpTo = (producedIndex + 1) % 10;
            sem_post(&ShmDataBlock->pIndexLock);
            produceCamData(&(ShmDataBlock->camdata[producedIndex]));
            sleep(2);
            dprintf(fd, "produced data with index %i for camera %i \n", dataIndex, chosenCamera->cameraId);
            fflush(stdout);

            sem_post(&ShmDataBlock->produced);
            dataIndex++;

            // dprintf(fd, "service is on for %i\n", chosenCamera->cameraId);
            // fflush(stdout);
            // sleep(2);
        }
        //shutdown service
        //I think we need a lock here because we off the sockets one by one
        ShmDataBlock->numberOfActiveCameras = ShmDataBlock->numberOfActiveCameras - 1;
        printf("The process is shut down\n");
        fflush(stdout);
        sem_post(&chosenCamera->gracefullFinishLock);
        exit(0);
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

void produceCamData(struct CamData *camdata)
{
    for (size_t i = 0; i < 10; i++)
    {
        for (size_t j = 0; j < 10; j++)
        {
            camdata->frames[i].frameData[j] = rand(); //here we should read from random
        }
    }
}
