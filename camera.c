
#include <pthread.h>
#include <semaphore.h>

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
