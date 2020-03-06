#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFSIZE 4096
#define FILLED 0
#define EMPTY 1

struct Frame
{
    int frameId;
    char frameData[5];
};

struct Camera
{
    pthread_mutex_t mutex;
    int status;
    int cameraId;
    struct Frame frames[10];
};

int main(int argc, char *argv[])
{
    key_t ShmKEY;
    int ShmID;
    struct Camera *ShmPTR;
    /*to get unigue identifier*/
    ShmKEY = ftok(".", 'x');
    /*to create shared memory with specific size and get its id*/
    ShmID = shmget(ShmKEY, sizeof(struct Camera), 0666);
    if (ShmID < 0)
    {
        printf("*** shmget error ***\n");
        exit(1);
    }
    /*attaching to the memory with pointer(struct pointer)*/
    ShmPTR = (struct Camera *)shmat(ShmID, NULL, 0);
    if ((int)ShmPTR == -1)
    {
        printf("*** shmat error (server) ***\n");
        exit(1);
    }
    for (;;)
    {
        pthread_mutex_lock(&(ShmPTR->mutex));

        if (ShmPTR->status != EMPTY)
        {
            ShmPTR->status = EMPTY;
            printf("camera index %i \n", ShmPTR->cameraId);
            for (size_t i = 0; i < 10; i++)
            {
                printf("%s\n", (ShmPTR->frames)[i].frameData);
                sleep(0.5);
            }
        }

        pthread_mutex_unlock(&(ShmPTR->mutex));
    }
}
