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

#define BUFSIZE 4096
#define ADD 2
#define REMOVE 3
#define STOP 5

int getCommand(char *buf)
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

int main(int argc, char *argv[])
{
    char input[50];
    int numberOfCameras;
    pid_t pid;
    //instructions
    printf("Welcome to Camera data load distributor \n");
    printf("----------------------------------------\n");
    printf("//The max number of cameras that can be parallely handled is 10\n");
    printf("----------------------------------------\n");
    //****
    printf("please enther the number of cameras that you want to handle: ");
    fgets(input, 50, stdin);
    numberOfCameras = atoi(input);

    printf("You've entered %i \n", numberOfCameras);
    fflush(stdin);
    pid = fork();

    if (pid > 0)
    {
        char buf[BUFSIZE];
        printf("--------------------------------\n");
        printf("//if you want to add additional cameras: enter \"add\"\n");
        printf("//if you want to remove camera: enter \"remove\"\n");
        printf("//if you want to stop the program: enter \"stop\"\n");
        printf("--------------------------------\n");

        while (fgets(buf, BUFSIZE, stdin) != NULL)
        {
            int commandType = getCommand(buf);
            if (commandType == ADD)
            {
                printf("You've entered add\n");
            }
            else if (commandType == REMOVE)
            {
                printf("You've entered remove\n");
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
        //here we should start handing child camera processes execlp???
    }
}
