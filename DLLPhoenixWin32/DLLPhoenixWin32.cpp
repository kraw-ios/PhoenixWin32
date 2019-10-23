#pragma once
#include "DLLPhoenixWin32.h"


DWORD writeMessage(ContrData *shm, SharedMessage *msg) {
	DWORD posW;
	WaitForSingleObject(shm->sharedMessages->hSemM, INFINITE);
	WaitForSingleObject(shm->hMutexMsg, INFINITE);
	
	shm->sharedMessages->numberOfMessages++;
	posW = shm->sharedMessages->posWrite;
	shm->sharedMessages->posWrite = (shm->sharedMessages->posWrite + 1) % Buffers;
	_tcscpy_s(shm->sharedMessages->arrayMessages[posW], msg->arrayMessages[0]);
	_tcscpy_s(shm->sharedMessages->name, msg->name);


	ReleaseMutex(shm->hMutexMsg);
	ReleaseSemaphore(shm->sharedMessages->hSemM, 1, NULL);

	return posW;
}

DWORD readMessage(ContrData *shm, SharedMessage *msg) {
	DWORD posR;
	WaitForSingleObject(shm->sharedMessages->hSemM, INFINITE);
	WaitForSingleObject(shm->hMutexMsg, INFINITE);
	posR = shm->sharedMessages->posRead;
	shm->sharedMessages->posRead = (shm->sharedMessages->posRead + 1) % Buffers;
	CopyMemory(msg, shm->sharedMessages, sizeof(SharedMessage));


	ReleaseMutex(shm->hMutexMsg);
	ReleaseSemaphore(shm->sharedMessages->hSemM, 1, NULL);

	return posR;
}

unsigned int peekMessage(ContrData *data) {
	unsigned msgNum;
	WaitForSingleObject(data->hMutexMsg, INFINITE);
	msgNum = data->sharedMessages->numberOfMessages;
	ReleaseMutex(data->hMutexMsg);

	return msgNum;
}


HANDLE createFileMapping() {
	HANDLE temp;

	temp = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Game), SharedMemoryName);
	if (!temp)
		_tprintf(TEXT("[ERROR] File Mapping not created!\n"));

	return temp;
}

Game * openFileMapping(HANDLE hMemory) {
	Game *temp;

	temp = (Game*)MapViewOfFile(hMemory, FILE_MAP_WRITE, 0, 0, sizeof(Game));
	if (!temp)
		_tprintf(TEXT("[ERROR] FileMap not oppened!\n"));

	return temp;
}


HANDLE createSemaphore(TCHAR name[TAM], int initialCount, int maximumCount) {
	HANDLE temp;

	temp = CreateSemaphore(NULL, initialCount, maximumCount, name);
	if (!temp)
		_tprintf(TEXT("[ERROR] Semaphore not created!\n"));

	return temp;
}


HANDLE createMutex(TCHAR name[TAM]) {
	HANDLE temp;

	temp = CreateMutex(NULL, FALSE, name);

	return temp;
}


BOOL MessagesSharedMemory(ContrData * data) {
	data->hMemoryMsg = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMessage), MsgSharedMemoryName);
	if (data->hMemoryMsg == NULL) {
		_tprintf(TEXT("[ERROR] Problem in message's shared memory\n"));
		return FALSE;
	}

	data->hMutexMsg = createMutex(MessagesMutexName); 
	if (data->hMutexMsg == NULL) {
		_tprintf(TEXT("[ERROR] Creating the mutex!\n"));
		return FALSE;
	}
	return TRUE;
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}