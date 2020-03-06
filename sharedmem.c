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
    char buf[BUFSIZE];
    key_t ShmKEY;
    int ShmID;
    struct Camera *ShmPTR;
    int index = 0;
    /*to get unigue identifier*/
    ShmKEY = ftok(".", 'x');
    /*to create shared memory with specific size and get its id*/
    ShmID = shmget(ShmKEY, sizeof(struct Camera), IPC_CREAT | 0666);
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

    while (fgets(buf, BUFSIZE, stdin) != NULL)
    {

        // If user types 'q' or 'Q', end the connection
        if (buf[0] == 'a' || buf[0] == 'A')
        {
            index++;
            printf("Camera with id %i was accepted\n", index);
            pid_t pid = fork();
            //here we are going to accept camera and deal with it currently we have one camera
            if (pid == 0)
            {
                char c = 'a';
                for (;;)
                {
                    pthread_mutex_lock(&(ShmPTR->mutex));
                    //critical section here we fill the frames
                    if (ShmPTR->status != FILLED)
                    {
                        ShmPTR->status = FILLED;
                        ShmPTR->cameraId = index;
                        for (size_t i = 0; i < 10; i++)
                        {
                            for (size_t j = 0; j < 4; j++)
                            {
                                (ShmPTR->frames[i]).frameData[j] = c;
                            }
                            (ShmPTR->frames[i]).frameData[4] = '\0';
                        }
                        sleep(2);
                        c++;
                    }

                    pthread_mutex_unlock(&(ShmPTR->mutex));
                }

                break;
            }
        }
    }
}
