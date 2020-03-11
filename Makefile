

all: manager cameraHandler gpuManager

manager: cameraManager.c 
	gcc -o manager cameraManager.c -pthread

cameraHandler: cameraHandler.c
	gcc -o cameraHandler cameraHandler.c -pthread 

gpuManager:	simpleGpuManager.c 
	gcc  -o gpumanager simpleGpuManager.c -pthread 

