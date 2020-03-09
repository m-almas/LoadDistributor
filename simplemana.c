#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUSY 7
#define EMPTY 5
#define FILLED 3

struct SharedInts
{
    //two semaphores we need
    sem_t produced; // 0-10
    sem_t consumed; // 0-10

    sem_t cIndexLock; // 0-1
    int consumedUpTo; // index of consumed up to (cumulative)
    sem_t pIndexLock; // 0-1 to access producedUpTo
    int producedUpTo; // index of produced up to (cumulative)
    int data[10];     //the actual data
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

int main(int argc, char *argv[])
{
    key_t ShmKEY;
    int ShmID;
    struct SharedInts *ShmPTR;
    /*to get unigue identifier*/
    ShmKEY = ftok(".", 'x');
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
    for (;;)
    {
        //something produced
        int i;
        int data;
        int consumedIndex;

        sem_wait(&ShmPTR->produced);
        sem_wait(&ShmPTR->cIndexLock);
        //get consumed index and mode it with 10
        consumedIndex = ShmPTR->consumedUpTo;
        ShmPTR->consumedUpTo = (consumedIndex + 1) % 10;
        sem_post(&ShmPTR->cIndexLock);

        //here we get the data and do our stuff
        sleep(1);
        data = ShmPTR->data[consumedIndex];
        printf("got the %i \n", data);
        fflush(stdout);

        sem_post(&ShmPTR->consumed);
    }
}
