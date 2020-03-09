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

#define BUSY 7
#define EMPTY 5
#define FILLED 3

struct Frames
{
    int frameId;
    int frameData[10]; // in the end it should be 1200*800 one dimensional array
    //also need checksum
    //isFinished? Do we need such variable??
};

struct CamData
{
    int camID;
    struct Frames frames[10];
};

//think about round robin when it comes into data
struct SharedInts
{
    //two semaphores we need
    sem_t produced; // 0-10
    sem_t consumed; // 0-10

    sem_t cIndexLock;           // 0-1
    int consumedUpTo;           // index of consumed up to (cumulative)
    sem_t pIndexLock;           // 0-1 to access producedUpTo
    int producedUpTo;           // index of produced up to (cumulative)
    struct CamData camdata[10]; //should be 60 frames of particular camera
};

int getFilledIndex(int *index, int length)
{
    for (size_t i = 0; i < length; i++)
    {
        if (index[i] == FILLED)
        {
            return i;
        }
    }
    return -1;
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

int main(int argc, char *argv[])
{
    key_t ShmKEY;
    int ShmID;
    pid_t pid;
    struct SharedInts *ShmPTR;
    /*to get unigue identifier*/
    ShmKEY = ftok("..", 'x');
    //allocating shared block
    ShmID = shmget(ShmKEY, sizeof(struct SharedInts), 0666);
    if (ShmID < 0)
    {
        printf("*** shmget error ***\n");
        exit(1);
    }

    ShmPTR = (struct SharedInts *)shmat(ShmID, NULL, 0);
    if ((int)ShmPTR == -1)
    {
        printf("*** shmat error ***\n");
        exit(1);
    }

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

            sem_wait(&ShmPTR->produced);
            sem_wait(&ShmPTR->cIndexLock);
            //get consumed index and mode it with 10
            consumedIndex = ShmPTR->consumedUpTo;
            ShmPTR->consumedUpTo = (consumedIndex + 1) % 10;
            sem_post(&ShmPTR->cIndexLock);

            //here we get the data and do our stuff
            sleep(1);
            consumeCamData(&(ShmPTR->camdata[consumedIndex]));
            fflush(stdout);

            sem_post(&ShmPTR->consumed);
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
