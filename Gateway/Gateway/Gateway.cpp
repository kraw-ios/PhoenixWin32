#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <conio.h>
#include <process.h>
#include "../../DLLPhoenixWin32/DLLPhoenixWin32.h"
#include "../../DLLPhoenixWin32/Structures.h"


HANDLE hSemS; // pode escrever
HANDLE hSemC; // pode ler
HANDLE hMutexS; 
HANDLE hMemory;


#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")
#define TAM_ARRAY_HANDLE 5

HANDLE hPipe[TAM_ARRAY_HANDLE];
HANDLE hP;

HANDLE hT; //-> Handle para a thread
HANDLE hTsendGame;
DWORD threadID;
char acabar = 0;
DWORD n;
TCHAR buf[256];
ContrData data;
SharedMessage request;


DWORD WINAPI atendeClientes(LPVOID param) {
	//CICLO PARA LEITURA DE PEDIDOS DO CLIENTE 
	HANDLE hPipe = (HANDLE)param;
	DWORD d;

	HANDLE IOReady;
	OVERLAPPED Ov;

	IOReady = CreateEvent(NULL, TRUE, FALSE, NULL);

	do {

		ZeroMemory(&Ov, sizeof(Ov));
		ResetEvent(IOReady);
		Ov.hEvent = IOReady;



		ReadFile(hPipe, &request, sizeof(request), &d, &Ov);
		WaitForSingleObject(IOReady, INFINITE);
		GetOverlappedResult(hPipe, &Ov, &d, FALSE);


		_tprintf(TEXT("\n[GATEWAY] Recebi pedido de cliente [%s] : '%s' \n"), request.name, request.arrayMessages[0]);

		writeMessage(&data, &request);
		
	} while (d > 0);

	return 0;
}

DWORD WINAPI aceitaClientes(LPVOID data) {

	while (acabar == 0) {
		
		hP = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, TAM_ARRAY_HANDLE, sizeof(Game), sizeof(Game), 1000, NULL); 
		if (hP == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ERROR] Creating Named Pipe"));
			exit(-1);
		}
		_tprintf(TEXT("[GATEWAY] Wait connection from player\n"));
		if (ConnectNamedPipe(hP, NULL)) {
			//Guardar handles no array se houver vaga
			for (int i = 0; i < TAM_ARRAY_HANDLE; i++) {
				if (hPipe[i] == INVALID_HANDLE_VALUE) {
					hPipe[i] = hP;
					_tprintf(TEXT("[GATEWAY] Player in\n"));
					break;
				}
			}
		}
		else {
			_tprintf(TEXT("[ERROR] Connecting Player\n"));
			CloseHandle(hP);
		}
		//lançar uma thread t2 que le pedidos do cliente .... AtendeCliente(hPipe)
		HANDLE ht = CreateThread(NULL, 0, atendeClientes, (LPVOID)hP, 0, NULL);

	}

	_tprintf(TEXT("[GATEWAY] Disconnecting pipe\n"));

	CloseHandle(hP);

	return 1;
}


DWORD WINAPI sendGameToClients(LPVOID data) {
	Game *game = (Game *)data;
	HANDLE IOReady;
	OVERLAPPED Ov;

	IOReady = CreateEvent(NULL, TRUE, FALSE, NULL);


	for (int i = 0; i < TAM_ARRAY_HANDLE; i++) {
		if (hPipe[i] != INVALID_HANDLE_VALUE) {

			ZeroMemory(&Ov, sizeof(Ov));
			ResetEvent(IOReady);
			Ov.hEvent = IOReady;


			if (!WriteFile(hPipe[i], game, sizeof(Game), &n, &Ov)) {
				_tprintf(TEXT("[ERROR] Writing on pipe %d! (WriteFile)\n"), i);
				return 0;
			}
			WaitForSingleObject(IOReady, INFINITE);
			GetOverlappedResult(hPipe, &Ov, &n, FALSE);

			_tprintf(TEXT("[GATEWAY] Sent %d bytes for pipe '%d'... (WriteFile)\n"), n, i);
		}
	}

	return 0;
}

int _tmain(void) {
	
	Game *shm;

#ifdef UNICODE 
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	//Colocar array handles com o valor INVALID_HANDLE_VALUE
	for (int i = 0; i < TAM_ARRAY_HANDLE; i++) {
		hPipe[i] = INVALID_HANDLE_VALUE;
	}

	hSemS = CreateSemaphore(NULL, 0, 1, ServerSemaphoreName);
	hSemC = CreateSemaphore(NULL, 0, 1, ClientSemaphoreName);

	hMutexS = CreateMutex(NULL, FALSE, ClientMutexName);

	if (hSemS == NULL)
		_tprintf(TEXT("[ERROR] Creating the writing semaphore!\n"));

	hMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Game), SharedMemoryName);

	shm = (Game*) MapViewOfFile(hMemory, FILE_MAP_WRITE, 0, 0, sizeof(Game));

	if (hMemory == NULL || hSemS == NULL)
		_tprintf(TEXT("[ERROR] Creating Windows objects!\n"));
	if (shm == NULL)
		_tprintf(TEXT("[ERROR] Mapping the shared memory!\n"));
	if (hMutexS == NULL)
		_tprintf(TEXT("[ERROR] Creating the server mutex"));


	data.hMemoryMsg = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, MsgSharedMemoryName);

	if (data.hMemoryMsg == NULL) {
		_tprintf(TEXT("\nFirst to access to the shared memory!\n"));
		if (!MessagesSharedMemory(&data)) {
			_tprintf(TEXT("\nImpossible to proceed (error: %d).\n"), GetLastError());
			exit(1);
		}
		_tprintf(TEXT("\nShared Memory and Mutex created!\n"));
	}
	else {
		data.hMutexMsg = OpenMutex(SYNCHRONIZE, TRUE, MessagesMutexName);
		if (data.hMutexMsg == NULL) {
			_tprintf(TEXT("\nMutex doesn't work (%d)! Exit!"), GetLastError());
			return FALSE;
		}
	}

	data.sharedMessages = (SharedMessage *)MapViewOfFile(data.hMemoryMsg, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMessage));

	if (data.sharedMessages == NULL) {
		_tprintf(TEXT("\nError creating the shared memory view (error: %d)!\n"), GetLastError());
		CloseHandle(data.hMemoryMsg);
		return 1;
	}

	data.sharedMessages->hSemM = CreateSemaphore(NULL, 1, 1, MessagesSemaphoreName);


	//Lançar thread "acceptClients"
	hT = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)aceitaClientes, NULL, 0, NULL);
	if (hT == NULL) {
		_tprintf(TEXT("[ERRO] Criar Threads aceitaClientes()"));
		exit(-1);
	}




	while (1) {

		hTsendGame = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)sendGameToClients, shm, 0, NULL);
		if (hTsendGame == NULL) {
			_tprintf(TEXT("[ERROR] Creating Thread sendGameToClients()"));
			exit(-1);
		}

		Sleep(500);

	}

	

	//informar a thread aceita clientes para terminar (var global -> flag)
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	acabar = 1;

	//Esperar pelo fim da thread Aceita Clientes
	WaitForSingleObject(hT, INFINITE);

	//Apagar named pi+es dos clientes que estavam ligados
	for (int i = 0; i < TAM_ARRAY_HANDLE; i++) {

		DisconnectNamedPipe(hPipe[i]);
		CloseHandle(hPipe[i]);
	}

	data.threadShouldContinue = 0;
	CloseHandle(hT);
	UnmapViewOfFile(data.sharedMessages);
	CloseHandle(data.hMemoryMsg);
	UnmapViewOfFile(shm);
	CloseHandle(hSemS);
	CloseHandle(hMutexS);
	CloseHandle(hMemory);
	CloseHandle(hSemC);

	return 0;
}