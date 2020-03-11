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

#define INPUT_BUFFSIZE 50
//comandStatus
#define ADD 2
#define REMOVE 3
#define STOP 5
//***

#define SOCKET_NUMBER 10

//helper functions
int getCommandType(char *buf);
void initCamData(struct DataBlock *ShmDataBlock);
void activateCameras(struct DataBlock *dataBlock, int numberOfCameras);
void initSeparateCameraSocket(struct CameraSocket *camera);
void initCameraSockets(struct DataBlock *ShmDataBlock);

void addChild(struct DataBlock *ShmDataBlock);
void removeChild(struct DataBlock *ShmDataBlock);
void stopChildProcesses(struct DataBlock *ShmDataBlock);

int main(int argc, char *argv[])
{
    char input[INPUT_BUFFSIZE]; //to take initial input(number of cameras we want to handle)
    int numberOfCameras;
    pid_t pid;
    //instructions
    printf("Welcome to Camera data load distributor \n");
    printf("----------------------------------------\n");
    printf("//The max number of cameras that can be parallely handled is 10\n");
    printf("----------------------------------------\n");
    printf("please enther the number of cameras that you want to handle: ");
    fgets(input, INPUT_BUFFSIZE, stdin);
    //****

    numberOfCameras = atoi(input);
    printf("You've entered %i \n", numberOfCameras);
    //allocating shared memory
    key_t ShmKEY;
    struct DataBlock *ShmDataBlock;
    int ShmID;
    //to get unigue identifier
    ShmKEY = ftok(".", 23);
    ShmID = shmget(ShmKEY, sizeof(struct DataBlock), IPC_CREAT | 0666);
    if (ShmID < 0)
    {
        printf("*** shmget error ***\n");
        exit(1);
    }
    ShmDataBlock = (struct DataBlock *)shmat(ShmID, NULL, 0);
    if ((int)ShmDataBlock == -1)
    {
        printf("*** shmat error ***\n");
        exit(1);
    }
    //****

    initCamData(ShmDataBlock);
    initCameraSockets(ShmDataBlock);
    activateCameras(ShmDataBlock, numberOfCameras);

    //for checking purposes
    for (size_t i = 0; i < 10; i++)
    {
        int camID = ShmDataBlock->cameraSockets[i].cameraId;
        printf("The cameraIDs: %i\n", camID);
        fflush(stdout);
    }
    //****
    pid = fork();

    if (pid > 0)
    {
        printf("--------------------------------\n");
        printf("//if you want to add additional cameras: enter \"add\"\n");
        printf("//if you want to remove camera: enter \"remove\"\n");
        printf("//if you want to stop the program: enter \"stop\"\n");
        printf("--------------------------------\n");
        while (fgets(input, INPUT_BUFFSIZE, stdin) != NULL)
        {
            int commandType = getCommandType(input);
            if (commandType == ADD)
            {
                addChild(ShmDataBlock);
            }
            else if (commandType == REMOVE)
            {
                removeChild(ShmDataBlock);
            }
            else if (commandType == STOP)
            {
                stopChildProcesses(ShmDataBlock);
                kill(pid, SIGTERM); // kill the cameraHandler process
                printf("all childs were stopped\n");
                //deallocation of shared memory block
                shmdt(ShmDataBlock);
                shmctl(ShmID, IPC_RMID, 0);
                exit(0);
            }
            else
            {
                printf("no such commands exists, please note that commands are case sensitive\n");
            }
        }
    }
    else
    {
        execl("cameraHandler", "cameraHandler", NULL);
    }
}

//******************************************************
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
    sem_init(&camera->cStatusLock, 2, 1);
    camera->cameraStatus = OFF;
}

void activateCameras(struct DataBlock *dataBlock, int numberOfCameras)
{
    dataBlock->numberOfActiveCameras = numberOfCameras;
    for (size_t i = 0; i < numberOfCameras; i++)
    {
        dataBlock->cameraSockets[i].cameraStatus = ON;
    }
}

void initCameraSockets(struct DataBlock *ShmDataBlock)
{

    sem_init(&ShmDataBlock->commandIndicator, 2, 0);
    for (size_t i = 0; i < SOCKET_NUMBER; i++)
    {
        initSeparateCameraSocket(&ShmDataBlock->cameraSockets[i]);
    }
}

/*
    look for sockets that status if OFF 
    then change it into on
    increase the number of active cameras
    send signal to controll block
    print added camera
*/
void addChild(struct DataBlock *ShmDataBlock)
{
    struct CameraSocket *camera;
    for (size_t i = 0; i < 10; i++)
    {
        camera = &ShmDataBlock->cameraSockets[i];

        sem_wait(&camera->cStatusLock);
        if (camera->cameraStatus == OFF)
        {
            camera->cameraStatus = ON;
            ShmDataBlock->numberOfActiveCameras++;
            ShmDataBlock->commandType = ADD;
            sem_post(&ShmDataBlock->commandIndicator); //this indicator should not be 2
            sem_post(&camera->cStatusLock);
            break;
        }
        sem_post(&camera->cStatusLock);
    }

    printf("camera with id: %i was added\n", camera->cameraId);
}

/*
    look for sockets that status if BUSY
    then change it into BUSY
    decrease the number of active cameras
    send signal to controll block
    wait for gracefull finish 
    print the removed camera
*/

void removeChild(struct DataBlock *ShmDataBlock)
{
    struct CameraSocket *camera;
    for (size_t i = 0; i < 10; i++)
    {
        camera = &ShmDataBlock->cameraSockets[i];
        sem_wait(&camera->cStatusLock);
        if (camera->cameraStatus == BUSY)
        {
            camera->cameraStatus = OFF;

            ShmDataBlock->numberOfActiveCameras--;
            ShmDataBlock->commandType = REMOVE;
            sem_post(&ShmDataBlock->commandIndicator);
            sem_post(&camera->cStatusLock);
            break;
        }
        sem_post(&camera->cStatusLock);
    }

    sem_wait(&camera->gracefullFinishLock);
    printf("The camera with ID: %i was removed\n", camera->cameraId);
}

void stopChildProcesses(struct DataBlock *ShmDataBlock)
{
    int currentActiveCameras = ShmDataBlock->numberOfActiveCameras;
    for (size_t i = 0; i < currentActiveCameras; i++)
    {
        removeChild(ShmDataBlock);
    }
}
