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
    sem_t produced; // 0-2
    sem_t consumed; // 0-2
    //to track which one produced
    int index[2];
    int length;
    //to secure the access of the above array
    sem_t lock; // 0 1

    int data[2]; //the actual data
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
        sem_wait(&ShmPTR->produced);
        int i;
        int data;
        int t;
        sem_wait(&ShmPTR->lock);
        i = getFilledIndex(ShmPTR->index, ShmPTR->length);
        if (i < 0)
        {
            printf("Error detected \n");
            fflush(stdout);
            exit(1);
        }

        ShmPTR->index[i] = BUSY;
        sem_post(&ShmPTR->lock);

        data = ShmPTR->data[i];
        printf("got the %i \n", data);
        fflush(stdout);
        sleep(1);

        sem_wait(&ShmPTR->lock);
        ShmPTR->index[i] = EMPTY;
        sem_post(&ShmPTR->lock);

        sem_post(&ShmPTR->consumed);
    }
}
