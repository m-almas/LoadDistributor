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

//think about round robin when it comes into data
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

int getEmptyIndex(int *index, int length)
{
    for (size_t i = 0; i < length; i++)
    {
        if (index[i] == EMPTY)
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
    ShmID = shmget(ShmKEY, sizeof(struct SharedInts), IPC_CREAT | 0666);
    if (ShmID < 0)
    {
        printf("*** shmget error ***\n");
        exit(1);
    }
    /*attaching to the memory with pointer*/
    ShmPTR = (struct SharedInts *)shmat(ShmID, NULL, 0);
    if ((int)ShmPTR == -1)
    {
        printf("*** shmat error ***\n");
        exit(1);
    }

    //**initialization of semaphores
    sem_init(&(ShmPTR->consumed), 2, 2);
    sem_init(&(ShmPTR->produced), 2, 0);
    sem_init(&(ShmPTR->lock), 2, 1);
    //** dealing with indexes
    ShmPTR->index[0] = EMPTY;
    ShmPTR->index[1] = EMPTY;
    ShmPTR->length = 2;
    /*******/
    pid_t pid = fork();
    if (pid == 0)
    {
        int data = 50;
        int i = 0;
        char buf[4096];
        for (;;)
        {
            sem_wait(&ShmPTR->consumed);
            sem_wait(&ShmPTR->lock);

            i = getEmptyIndex(ShmPTR->index, ShmPTR->length);
            if (i < 0)
            {
                printf("Error detected \n");
                fflush(stdout);
                exit(1);
            }

            ShmPTR->index[i] = BUSY;

            sem_post(&ShmPTR->lock);

            ShmPTR->index[i] = 150;
            ShmPTR->data[i] = data;
            printf("produced %i\n", data);
            fflush(stdout);
            sleep(2);

            sem_wait(&ShmPTR->lock);

            ShmPTR->index[i] = FILLED;

            sem_post(&ShmPTR->lock);
            sem_post(&ShmPTR->produced);
            data++;
        }
    }
    else
    {
        waitpid(pid, NULL, 0);
        printf("%i \n", ShmPTR->data[0]);
        printf("%i \n", ShmPTR->data[1]);
    }
}
