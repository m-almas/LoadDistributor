# Load distribution with shared memory 

## The main parts: 
1) cameraHandler.c 
2) cameraManager.c 
3) SimpleGpuManager.c
4) dataBlock.h

# dataBlock.h
The purpose of this file is to describe the memory block that will be shared among processes.
Let's stop on each variable: 
* DataBlock Shared data block. Contains everything the program needs
* numberOfActiveCameras -> it is self explanatory to track number of active cameras that are producing
* commandType -> this is variable that cameraManager uses to signal cameraHandler (more on this further)
* commandIndicator -> this is semaphore to synchronize the access to commandType 
* produced -> semaphore, indicates number of produced data. Initially set to 0 
* consumed -> semaphore, indicates number of consumed data. Initially set to 20 (MAX_CAM_NUMBER * 2)
* consumedUpTo -> the value of this integer indicates that up to which value of array of camdata gpu already consumed (encoded) (cumulative)
* producedUpTo -> the value indicates that up to which value of array of camdata produced (cumulative)
* cIndexLock, pIndexLock -> semaphore, to sync the access to respective integers
* struct cameraSockets -> the entity that mock camera will use to write data 
* struct camData -> the actual camera data, each camData have 60 frames of particular camera. 

## CameraSocket
* cameraID -> id of connected camera 
* cameraStatus -> status of the socket (BUSY, ON, OFF)
* gracefullFinishLock -> lock whose purpose is to ensure proper shutdown of the socket. We should not power off in the middle of the writing process. 

# cameraManager.c 
1) allocates shared memory 
2) get from user initial active camera value 
3) cooperate with control block of cameraHandler to (ADD, REMOVE, STOP)
4) cooperation happens with commandIndicator lock 

### how add works? 

