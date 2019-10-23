#pragma once
#include "Structures.h"


#define STRINGSIZE 255
#define Buffers 10
#define BufferSize 100



typedef struct {

	TCHAR arrayMessages[Buffers][BufferSize];
	TCHAR name[BufferSize];
	unsigned numberOfMessages = 0;
	int posWrite = 0;
	int posRead = 0;
	HANDLE hSemM;
}SharedMessage;

typedef struct {
	HANDLE hMemoryMsg;
	SharedMessage *sharedMessages;
	HANDLE hMutexMsg;
	TCHAR playerName[STRINGSIZE];
	int threadShouldContinue;
}ContrData;


TCHAR MessagesSemaphoreName[] = TEXT("Messages Semaphore");
TCHAR MessagesMutexName[] = TEXT("Messages Mutex");

TCHAR SharedMemoryName[] = TEXT("Shared Memory");
TCHAR MsgSharedMemoryName[] = TEXT("Msg Shared Memory");

TCHAR ServerSemaphoreName[] = TEXT("Server Semaphore");
TCHAR ClientSemaphoreName[] = TEXT("Client Semaphore");
TCHAR ServerMutexName[] = TEXT("Server Mutex");
TCHAR ClientMutexName[] = TEXT("Client Mutex");

TCHAR MutexBulletName[] = TEXT("Bullet Mutex");


int index = 0;

#define PHOENIXWIN32_API __declspec(dllexport)

extern "C"
{

	PHOENIXWIN32_API DWORD writeMessage(ContrData *shm, SharedMessage *msg);
	PHOENIXWIN32_API DWORD readMessage(ContrData *shm, SharedMessage *msg);
	PHOENIXWIN32_API unsigned peekMessage(ContrData *data);
	PHOENIXWIN32_API HANDLE createFileMapping();
	PHOENIXWIN32_API Game * openFileMapping(HANDLE hMemory);
	PHOENIXWIN32_API BOOL MessagesSharedMemory(ContrData *data);
	PHOENIXWIN32_API HANDLE createSemaphore(TCHAR name[TAM], int initialCount, int maximumCount);
	PHOENIXWIN32_API HANDLE createMutex(TCHAR name[TAM]);
}
