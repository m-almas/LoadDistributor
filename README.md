# Load distribution with shared memory 

## Usage 
* ``` make all ```
* ``` ./manager ```
* enter the number of active cameras 
* then on separate terminal window run ``` ./gpumanager ```
* the outputs are generated in cameralogs.txt and gpulogs.txt
* to add new camera enter ```add``` into manager prompt 
* to remove enter ```remove```
* to stop enter ```stop```

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

### How add works? 
1) we find camera whose status is OFF (here I should mention that, we find it just by iteration, however it is easy to configure so that we can choose which camera to add and remove)
2) after that we change its status into ON
3) specify the command type
4) and add 1 to the semaphore value -> (it acts like a signal for the control block of cameraHandler)

### How remove works? 
1) we find camera whose status is BUSY (here I should mention that, we find it just by iteration, however it is easy to configure so that we can choose which camera to add and remove)
2) after that we change its status into OFF
3) specify the command type
4) and add 1 to the semaphore value -> (it acts like a signal for the control block of cameraHandler)
5) then wait for the gracefullShutdown -> only then we can proceed further

# cameraHandler.c 
It has three parts: 
1) init block -> to activate cameras initially (number of active cameras that we get from user)
2) control block -> with the help of command indicator the block will cooperate with manager 
3) do something block -> where active camera writes data into allocated blocks

# SimpleGpuManager.c 
1) two GPU encoders 
2) I've assumed that it will presumably spend 0.2 seconds for 60 frames -> considering 300fps 
3) It will cooperate with cameraHandlers with just one lock (produced) 
4) cIndex lock is required to cooperate within those 2 processes

