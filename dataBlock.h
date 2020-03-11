#ifndef BLOCK_CAM_DATA
#define BLOCK_CAM_DATA

#include <semaphore.h>

//camera statuses
#define ON 2
#define OFF 3
#define BUSY 5
#define MAX_CAM_NUMBER 10
//****

struct CameraSocket
{
    int cameraId;
    int cameraStatus;
    sem_t cStatusLock;         //lock for status
    sem_t gracefullFinishLock; //to indicate that process is done. Initial value is 0;
};

struct Frame
{
    int frameId;
    int frameData[10]; //this is going to be 1200*800 array
    //checksum
    //isFinished? Do we need such variable?? to handle error cases?
};

struct CamData
{
    int camID;               //id of the camera currently it is random
    struct Frame frames[10]; // frames of particular camera, in the end it will be 60
};

struct DataBlock
{
    struct CameraSocket
        cameraSockets[MAX_CAM_NUMBER]; // the cameras will connect to these sockets
    int numberOfActiveCameras;         // should resemble number active processes
                                       // guess this should also have lock
    sem_t commandIndicator;            // initial value is 0. it's purpose is
                                       // to indicate about new command
    int commandType;                   // indicated commandType

    //The camData part
    sem_t produced; // 0-10
    sem_t consumed; // 0-10

    sem_t cIndexLock; // 0-1 to access consumedUpTo
    int consumedUpTo; // index of consumedUpTo (cumulative)
    sem_t pIndexLock; // 0-1 to access producedUpTo
    int producedUpTo; // index of producedUpTo (cumulative)
    struct CamData
        camdata[MAX_CAM_NUMBER * 2]; // the data read from cameras will end up in this array
                                     // each camdata have 60 frames of particular camera
};

#endif
