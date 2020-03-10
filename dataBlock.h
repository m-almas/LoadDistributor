#ifndef BLOCK_CAM_DATA
#define BLOCK_CAM_DATA

#include <semaphore.h>

struct CameraSocket
{
    int cameraId;
    int serviced;              // to indicate that camera is taken by some process and already serviced
    int on;                    //0-1 0 means off, 1 means on
    sem_t onOffLock;           //lock for on off variable 0-1
    sem_t gracefullFinishLock; //to indicate that process is done. Initial value is 0;
    sem_t servicedLock;        //
};

struct Frame
{
    int frameId;
    int frameData[10]; // in the end it should be 1200*800 one dimensional array
    //also need checksum
    //isFinished? Do we need such variable??
};

struct CamData
{
    int camID;
    struct Frame frames[10];
};

struct DataBlock
{
    //create so that if process is created then there exists camera with on state and not serviced.
    //by other processes
    //The cameras itself
    struct CameraSocket cameras[10]; //the idea is that every process will iterate through this
    //array and look for on camera
    int numberOfActiveCameras; //should resemble number active processes // guess this also should have lock
    sem_t commandStatus;       //initial is 0 post is done on manager process.
    int commandType;
    //The camData part
    sem_t produced; // 0-10
    sem_t consumed; // 0-10

    sem_t cIndexLock;           // 0-1
    int consumedUpTo;           // index of consumed up to (cumulative)
    sem_t pIndexLock;           // 0-1 to access producedUpTo
    int producedUpTo;           // index of produced up to (cumulative)
    struct CamData camdata[10]; //should be 60 frames of particular camera
};

#endif
