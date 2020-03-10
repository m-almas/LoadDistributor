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
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "dataBlock.h"

#define BUFSIZE 4096
#define ADD 2
#define REMOVE 3
#define STOP 5

//helper functions
int getCommandType(char *buf);
void *initSharedMemory(key_t ShmKEY);
void initCamData(struct DataBlock *ShmDataBlock);
void activateCameras(struct DataBlock *dataBlock, int numberOfCameras);
void initSeparateCameraSocket(struct CameraSocket *camera);
void initCameraSockets(struct DataBlock *ShmDataBlock, int numberOfCameras);

int main(int argc, char *argv[])
{
    char input[50]; //to take initial input(number of cameras we want to handle)
    int numberOfCameras;
    pid_t pid;
    //instructions
    printf("Welcome to Camera data load distributor \n");
    printf("----------------------------------------\n");
    printf("//The max number of cameras that can be parallely handled is 10\n");
    printf("----------------------------------------\n");
    printf("please enther the number of cameras that you want to handle: ");
    fgets(input, 50, stdin);
    //****

    numberOfCameras = atoi(input);
    printf("You've entered %i \n", numberOfCameras);
    //allocating shared memory
    key_t ShmKEY;
    struct DataBlock *ShmDataBlock;
    //to get unigue identifier
    ShmKEY = ftok(".", 23);
    ShmDataBlock = (struct DataBlock *)initSharedMemory(ShmKEY);
    //****

    initCamData(ShmDataBlock);
    initCameraSockets(ShmDataBlock, 10);
    activateCameras(ShmDataBlock, numberOfCameras);

    //for checking purposes
    for (size_t i = 0; i < 10; i++)
    {
        int camID = ShmDataBlock->cameras[i].cameraId;
        printf("The cameraIDs: %i\n", camID);
        fflush(stdout);
    }
    pid = fork();

    if (pid > 0)
    {
        printf("--------------------------------\n");
        printf("//if you want to add additional cameras: enter \"add\"\n");
        printf("//if you want to remove camera: enter \"remove\"\n");
        printf("//if you want to stop the program: enter \"stop\"\n");
        printf("--------------------------------\n");
        while (fgets(input, 50, stdin) != NULL)
        {
            int commandType = getCommandType(input);
            if (commandType == ADD)
            {
                struct CameraSocket *camera;
                // look for off cameraSockets
                // and on it (initialize with new variable)
                // then increment the number of active cameras in order to limit
                // after that you should post commandStatus and set the command into ADD
                for (size_t i = 0; i < 10; i++)
                {
                    camera = &ShmDataBlock->cameras[i];

                    sem_wait(&camera->onOffLock);
                    if (!(camera->on))
                    {
                        camera->on = 1;
                        camera->serviced = 0;
                        ShmDataBlock->commandType = ADD;
                        sem_post(&ShmDataBlock->commandStatus); //this status should not be 2
                        sem_post(&camera->onOffLock);
                        break;
                    }
                    sem_post(&camera->onOffLock);
                }

                printf("You've entered add\n");
            }
            else if (commandType == REMOVE)
            {
                struct CameraSocket *camera;
                for (size_t i = 0; i < 10; i++)
                {
                    camera = &ShmDataBlock->cameras[i];
                    sem_wait(&camera->onOffLock);
                    if (camera->on)
                    {
                        camera->on = 0;

                        ShmDataBlock->commandType = REMOVE;
                        sem_post(&ShmDataBlock->commandStatus);
                        sem_post(&camera->onOffLock);
                        break;
                    }
                    sem_post(&camera->onOffLock);
                }

                sem_wait(&camera->gracefullFinishLock);
                printf("The camera with ID: %i was removed\n", camera->cameraId);
                //first we want to find the camera that is on and off it if it serviced wait for gracefull
                //shutdown
                //first we wait for the on off lock
                //then set on to 0 meaning that it is offed
                //after that we wait for gracefull shutdown
                //the remove completed
                //do we need to decrease the number of active cameras??
                //or process should be responsible to this?? I guess process
                // sem_wait(&ShmDataBlock->cameras[0].onOffLock);
                // ShmDataBlock->cameras[0].on = 0; //this means that we've offed the camera
                // //consider this as a signal
                // ShmDataBlock->commandType = REMOVE;
                // sem_post(&ShmDataBlock->commandStatus);

                // sem_post(&ShmDataBlock->cameras[0].onOffLock);
                // //do we need gracefullshutdown? Yes we can not add unless we are sure that it is shutdown
                // sem_wait(&ShmDataBlock->cameras[0].gracefullFinishLock);
                // printf("The camera with ID: %i was removed\n", ShmDataBlock->cameras[0].cameraId);
                // //here it is off and can be replaced with other process.
                //how we can check it?
            }
            else if (commandType == STOP)
            {
                printf("You've entered stop\n");
            }
            else
            {
                printf("no such commands exists, please note that commands are case sensitive\n");
            }
        }
    }
    else
    {
        int status;
        //initblock
        for (size_t i = 0; i < ShmDataBlock->numberOfActiveCameras; i++)
        {
            pid = fork();

            if (pid == 0)
            {
                break;
            }
        }

        //initblock
        //controllblock
        if (pid > 0)
        {
            for (;;)
            {
                sem_wait(&ShmDataBlock->commandStatus);
                if (ShmDataBlock->commandType == REMOVE)
                {
                    wait(&status);
                }
                else if (ShmDataBlock->commandType == ADD)
                {
                    pid = fork();
                }
                else
                {
                    break; //in this case we will wait out all childs to finish and exit
                }
                if (pid == 0)
                {
                    break;
                }
            }
        }
        //controllblock

        //do somethingblock
        if (pid == 0)
        {
            int fd;
            fd = open("logs.txt", O_CREAT | O_APPEND | O_WRONLY, 0644);
            struct CameraSocket *chosenCamera;
            for (size_t i = 0; i < 10; i++)
            {
                chosenCamera = &ShmDataBlock->cameras[i];
                sem_wait(&chosenCamera->onOffLock);
                if (chosenCamera->on && !(chosenCamera->serviced)) //problem is here
                {
                    break;
                }
                sem_post(&chosenCamera->onOffLock);
            }
            chosenCamera->serviced = 1;
            sem_post(&chosenCamera->onOffLock);
            //camera is chosen for service
            for (;;)
            {
                sem_wait(&chosenCamera->onOffLock);
                if (!(chosenCamera->on))
                {
                    sem_post(&chosenCamera->onOffLock);
                    break;
                }
                sem_post(&chosenCamera->onOffLock);
                dprintf(fd, "service is on for %i\n", chosenCamera->cameraId);
                fflush(stdout);
                sleep(2);
            }
            //I think we need a lock here
            ShmDataBlock->numberOfActiveCameras = ShmDataBlock->numberOfActiveCameras - 1;
            printf("The process is shut down\n");
            fflush(stdout);
            sem_post(&chosenCamera->gracefullFinishLock);
            exit(1);
        }
    }
}

int getCommandType(char *buf)
{
    if (strncmp(buf, "add", 3) == 0)
    {
        return ADD;
    }
    else if (strncmp(buf, "remove", 6) == 0)
    {
        return REMOVE;
    }
    else if (strncmp(buf, "stop", 4) == 0)
    {
        return STOP;
    }
    return -1;
}

void *initSharedMemory(key_t ShmKEY)
{
    int ShmID;
    struct DataBlock *ShmPTR;

    ShmID = shmget(ShmKEY, sizeof(struct DataBlock), IPC_CREAT | 0666);
    if (ShmID < 0)
    {
        printf("*** shmget error ***\n");
        exit(1);
    }
    /*attaching to the memory with pointer*/
    ShmPTR = (struct DataBlock *)shmat(ShmID, NULL, 0);
    if ((int)ShmPTR == -1)
    {
        printf("*** shmat error ***\n");
        exit(1);
    }
    return ShmPTR;
}

void initCamData(struct DataBlock *ShmDataBlock)
{

    sem_init(&ShmDataBlock->consumed, 2, 10);
    sem_init(&ShmDataBlock->produced, 2, 0);
    sem_init(&ShmDataBlock->cIndexLock, 2, 1);
    sem_init(&ShmDataBlock->pIndexLock, 2, 1);
    ShmDataBlock->producedUpTo = 0;
    ShmDataBlock->consumedUpTo = 0;
}

void initSeparateCameraSocket(struct CameraSocket *camera)
{
    camera->cameraId = rand();
    sem_init(&camera->gracefullFinishLock, 2, 0);
    sem_init(&camera->servicedLock, 2, 1);
    camera->serviced = 0;
    sem_init(&camera->onOffLock, 2, 1);
    camera->on = 0;
}

void activateCameras(struct DataBlock *dataBlock, int numberOfCameras)
{
    dataBlock->numberOfActiveCameras = numberOfCameras;
    for (size_t i = 0; i < numberOfCameras; i++)
    {
        dataBlock->cameras[i].on = 1;
    }
}

void initCameraSockets(struct DataBlock *ShmDataBlock, int numberOfCameras)
{

    sem_init(&ShmDataBlock->commandStatus, 2, 0);
    for (size_t i = 0; i < numberOfCameras; i++)
    {
        initSeparateCameraSocket(&ShmDataBlock->cameras[i]);
    }
}
