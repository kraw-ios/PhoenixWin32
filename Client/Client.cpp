#include "stdafx.h"
#include "Client.h"
#include "atlstr.h"
#include "../DLLPhoenixWin32/DLLPhoenixWin32.h"
#include "../DLLPhoenixWin32/Structures.h"

#define MAX_LOADSTRING 100

#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")

// Variáveis Globais:
HINSTANCE hInst;                                // instância atual
WCHAR szTitle[MAX_LOADSTRING];                  // O texto da barra de título
WCHAR szWindowClass[MAX_LOADSTRING];            // o nome da classe da janela principal

//TCHAR request[256];
SharedMessage request;

HANDLE hPipe;
HANDLE IOReady;
OVERLAPPED Ov;
DWORD d;
HANDLE hT;
Game game;
HWND hWnd;

UINT x = 0, y = 0;
// Declarações de encaminhamento de funções incluídas nesse módulo de código:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Login(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    End(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI receiveGame(LPVOID param);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow){

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Coloque código aqui.

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif


    //Inicializar cadeias de caracteres globais
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

	IOReady = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
		exit(-1);
	}

	hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, /*FILE_ATTRIBUTE_NORMAL*/ 0 | FILE_FLAG_OVERLAPPED, NULL); // acrescentei o |Generic wirte e o file_attribute_normal por o que escrevi
	if (!hPipe) {
		exit(-1);
	}

	_tcscpy_s(request.name, TEXT("Player"));

	DWORD mode = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);

	hT = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveGame, NULL, 0, NULL);



    // Realiza a inicialização da aplicação:
    if (!InitInstance (hInstance, nCmdShow)){
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));

    MSG msg;

    // Loop da mensagem principal:
    while (GetMessage(&msg, nullptr, 0, 0)){
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

			

        }
    }

    return (int) msg.wParam;
}



//
//  FUNÇÃO: MyRegisterClass()
//
//  PROPÓSITO: Registra a classe de janela.
//
ATOM MyRegisterClass(HINSTANCE hInstance){
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CLIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNÇÃO: InitInstance(HINSTANCE, int)
//
//   PROPÓSITO: Salva gerenciador de instância e cria janela principal
//
//   COMENTÁRIOS:
//
//        Nesta função, o gerenciador de instâncias é salvo em uma variável global e
//        a janela do programa principal é criada e apresentada.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow){
   hInst = hInstance; // Salvar gerenciador de instância na variável global

	hWnd = CreateWindowW(szWindowClass, TEXT("PhoenixWin32"), WS_OVERLAPPEDWINDOW,
      50, 50, 700, 680, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd){
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNÇÃO: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PROPÓSITO:  Processa as mensagens para a janela principal.
//
//  WM_COMMAND - processar o menu do aplicativo
//  WM_PAINT - Pintar a janela principal
//  WM_DESTROY - postar uma mensagem de saída e retornar
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	
	PAINTSTRUCT ps;

	static HDC hdc = NULL;
	static HDC auxDC = NULL;
	static HBRUSH bg = NULL;
	static HBITMAP auxBM = NULL;

	static int nX = 0, nY = 0;


	// bitmap

	static HBITMAP hNave[10];
	static BITMAP bmNave;
	static HDC hdcNave;

	static HBITMAP hPlayer;
	static BITMAP bmPlayer;
	static HDC hdcPlayerShip;

	static HBITMAP hPlayerBullet;
	static BITMAP bmPlayerBullet;
	static HDC hdcPlayerBullet;

	static HBITMAP hEnemyBullet;
	static BITMAP bmEnemyBullet;
	static HDC hdcEnemyBullet;

	static HBITMAP hPowerup1;
	static BITMAP bmPowerup1;
	static HDC hdcPowerup1;

	static HBITMAP hPowerup2;
	static BITMAP bmPowerup2;
	static HDC hdcPowerup2;

	static HBITMAP hPowerup3;
	static BITMAP bmPowerup3;
	static HDC hdcPowerup3;

	static HBITMAP hPowerup4;
	static BITMAP bmPowerup4;
	static HDC hdcPowerup4;

	static HBITMAP hPowerup5;
	static BITMAP bmPowerup5;
	static HDC hdcPowerup5;

	static HBITMAP hPowerup6;
	static BITMAP bmPowerup6;
	static HDC hdcPowerup6;

	TCHAR scoreLabel[30] = TEXT(" ");
	TCHAR levelLabel[30] = TEXT(" ");

	switch (message){
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Interpreta as seleções do menu:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_CLOSE:
		{
			int res = MessageBox(hWnd, TEXT("Pretende sair?"), TEXT("Confirmar"), MB_YESNO);
			if (res == IDYES) {
				_tcscpy_s(request.arrayMessages[0], TEXT("Exit"));
				//request[_tcslen(request) - 1] = '\0';
				ZeroMemory(&Ov, sizeof(Ov));
				ResetEvent(IOReady);
				Ov.hEvent = IOReady;

				WriteFile(hPipe, &request, sizeof(request), &d, &Ov);
				WaitForSingleObject(IOReady, INFINITE);
				GetOverlappedResult(hPipe, &Ov, &d, FALSE);

				DestroyWindow(hWnd);
			}
				
		}
		break;

	case WM_CREATE:
		DialogBox(hInst, MAKEINTRESOURCE(IDD_LOGINBOX), hWnd, Login);

		bg = CreateSolidBrush(RGB(0, 0, 0));
		nX = GetSystemMetrics(SM_CXSCREEN);
		nY = GetSystemMetrics(SM_CYSCREEN);

		// PREPARA 'BITMAP' E ASSOCIA A UM 'DC' EM MEMORIA... 
		hdc = GetDC(hWnd);
		auxDC = CreateCompatibleDC(hdc);
		auxBM = CreateCompatibleBitmap(hdc, nX, nY);
		SelectObject(auxDC, auxBM);
		SelectObject(auxDC, bg);
		PatBlt(auxDC, 0, 0, nX, nY, PATCOPY);
		ReleaseDC(hWnd, hdc);

		for (int i = 0; i < game.numberEnem; i++) {
			hNave[i] = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP12), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

			hdc = GetDC(hWnd);
			GetObject(hNave[i], sizeof(bmNave), &bmNave);
			hdcNave = CreateCompatibleDC(hdc);
			SelectObject(hdcNave, hNave[i]);
			ReleaseDC(hWnd, hdc);

		}

		hPlayer = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP11), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
		hdc = GetDC(hWnd);
		GetObject(hPlayer, sizeof(bmPlayer), &bmPlayer);
		hdcPlayerShip = CreateCompatibleDC(hdc);
		SelectObject(hdcPlayerShip, hPlayer);


		hEnemyBullet = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP3), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

		hdc = GetDC(hWnd);
		GetObject(hEnemyBullet, sizeof(bmPlayerBullet), &bmPlayerBullet);
		hdcPlayerBullet = CreateCompatibleDC(hdc);
		SelectObject(hdcPlayerBullet, hEnemyBullet);
		ReleaseDC(hWnd, hdc);


		hPlayerBullet = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP4), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

		hdc = GetDC(hWnd);
		GetObject(hPlayerBullet, sizeof(bmEnemyBullet), &bmEnemyBullet);
		hdcEnemyBullet = CreateCompatibleDC(hdc);
		SelectObject(hdcEnemyBullet, hPlayerBullet);
		ReleaseDC(hWnd, hdc);


		
		hPowerup1 = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP5), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

		hdc = GetDC(hWnd);
		GetObject(hPowerup1, sizeof(bmPowerup1), &bmPowerup1);
		hdcPowerup1 = CreateCompatibleDC(hdc);
		SelectObject(hdcPowerup1, hPowerup1);
		ReleaseDC(hWnd, hdc);

		hPowerup2 = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP6), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

		hdc = GetDC(hWnd);
		GetObject(hPowerup2, sizeof(bmPowerup2), &bmPowerup2);
		hdcPowerup2 = CreateCompatibleDC(hdc);
		SelectObject(hdcPowerup2, hPowerup2);
		ReleaseDC(hWnd, hdc);

		hPowerup3 = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP7), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

		hdc = GetDC(hWnd);
		GetObject(hPowerup3, sizeof(bmPowerup3), &bmPowerup3);
		hdcPowerup3 = CreateCompatibleDC(hdc);
		SelectObject(hdcPowerup3, hPowerup3);
		ReleaseDC(hWnd, hdc);

		hPowerup4 = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP8), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

		hdc = GetDC(hWnd);
		GetObject(hPowerup4, sizeof(bmPowerup4), &bmPowerup4);
		hdcPowerup4 = CreateCompatibleDC(hdc);
		SelectObject(hdcPowerup4, hPowerup4);
		ReleaseDC(hWnd, hdc);

		hPowerup5 = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP9), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

		hdc = GetDC(hWnd);
		GetObject(hPowerup1, sizeof(bmPowerup5), &bmPowerup5);
		hdcPowerup5 = CreateCompatibleDC(hdc);
		SelectObject(hdcPowerup5, hPowerup5);
		ReleaseDC(hWnd, hdc);

		hPowerup6 = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP10), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

		hdc = GetDC(hWnd);
		GetObject(hPowerup6, sizeof(bmPowerup6), &bmPowerup6);
		hdcPowerup6 = CreateCompatibleDC(hdc);
		SelectObject(hdcPowerup6, hPowerup6);
		ReleaseDC(hWnd, hdc);


		break;

	case WM_KEYDOWN:
		// ANDAR DO DEFENSOR
		
		if (wParam == VK_LEFT) {
			//copiar para a string esquerda
			_tcscpy_s(request.arrayMessages[0], TEXT("Left"));
			//request[_tcslen(request) - 1] = '\0';
			ZeroMemory(&Ov, sizeof(Ov));
			ResetEvent(IOReady);
			Ov.hEvent = IOReady;

			WriteFile(hPipe, &request, sizeof(request), &d, &Ov);
			WaitForSingleObject(IOReady, INFINITE);
			GetOverlappedResult(hPipe, &Ov, &d, FALSE);

			InvalidateRect(hWnd, NULL, FALSE);
		}
		if (wParam == VK_RIGHT) {
			_tcscpy_s(request.arrayMessages[0], TEXT("Right"));
			//request[_tcslen(request) - 1] = '\0';
			ZeroMemory(&Ov, sizeof(Ov));
			ResetEvent(IOReady);
			Ov.hEvent = IOReady;

			WriteFile(hPipe, &request, sizeof(request), &d, &Ov);
			WaitForSingleObject(IOReady, INFINITE);
			GetOverlappedResult(hPipe, &Ov, &d, FALSE);

			InvalidateRect(hWnd, NULL, FALSE);
		}
		// TIROS DO DEFENSOR
		if (wParam == VK_SPACE) {
			_tcscpy_s(request.arrayMessages[0], TEXT("Fire"));
			//request[_tcslen(request) - 1] = '\0';
			ZeroMemory(&Ov, sizeof(Ov));
			ResetEvent(IOReady);
			Ov.hEvent = IOReady;

			WriteFile(hPipe, &request, sizeof(request), &d, &Ov);
			WaitForSingleObject(IOReady, INFINITE);
			GetOverlappedResult(hPipe, &Ov, &d, FALSE);

			InvalidateRect(hWnd, NULL, FALSE);
		}

		break;

    case WM_PAINT:
        {
			
			PatBlt(auxDC, 0, 0, nX, nY, PATCOPY);

			PatBlt(auxDC, 0, 0, nX, nY, PATCOPY);
			SetStretchBltMode(auxDC, BLACKONWHITE);

			_stprintf(levelLabel, _T("Level: %d"), game.level);
			TextOut(auxDC, 0, 600, levelLabel, 10);

			for (int i = 0; i < game.numberEnem; i++) {
				if(game.enemies[i].ative == 1)
					StretchBlt(auxDC, game.enemies[i].pos.x, game.enemies[i].pos.y, game.enemies[i].dimX, game.enemies[i].dimY, hdcNave, 0, 0, bmNave.bmWidth, bmNave.bmHeight, SRCCOPY);
				
			}
			
			for (int a = 0; a < 3; a++) {
				if (_tcscmp(game.players[a].username, TEXT("\0")) != 0) {
					StretchBlt(auxDC, game.players[a].pos.x, game.players[a].pos.y, game.players[a].dimX, game.players[a].dimY, hdcPlayerShip, 0, 0, bmPlayer.bmWidth, bmPlayer.bmHeight, SRCCOPY);
				}
			}
			for (int a = 0; a < 3; a++) {
				if (_tcscmp(game.players[a].username, TEXT("\0")) != 0) {
					for (int x = 0; x < 10; x++) {
						if(game.players[a].bullet[x].usage == 1)
							StretchBlt(auxDC, game.players[a].bullet[x].pos.x, game.players[a].bullet[x].pos.y, 3, 5, hdcPlayerBullet, 0, 0, bmPlayerBullet.bmWidth, bmPlayerBullet.bmHeight, SRCCOPY);
							_stprintf(scoreLabel,  _T("Score: %d  Lives: %d"), game.players[a].score,game.players[a].lives);
							TextOut(auxDC, 550, 600, scoreLabel, 20);
					}
					
				}
			}

			for (int a = 0; a < game.numberEnem; a++) {
					for (int x = 0; x < 5; x++) {
						if (game.enemies[a].bullet[x].usage == 1)
							StretchBlt(auxDC, game.enemies[a].bullet[x].pos.x, game.enemies[a].bullet[x].pos.y, 3, 5, hdcEnemyBullet, 0, 0, bmEnemyBullet.bmWidth, bmEnemyBullet.bmHeight, SRCCOPY);
					}
			}

			for (int a = 0; a < game.number_powerups; a++) {
				if (game.powerUps[a].usage == 1) {
					if (_tcscmp(game.powerUps[a].type, TEXT("SHIELD")) == 0)
						StretchBlt(auxDC, game.powerUps[a].pos.x, game.powerUps[a].pos.y, 5, 5, hdcPowerup1, 0, 0, bmEnemyBullet.bmWidth, bmEnemyBullet.bmHeight, SRCCOPY);
					if (_tcscmp(game.powerUps[a].type, TEXT("ENEMIES_FASTER")) == 0)
						StretchBlt(auxDC, game.powerUps[a].pos.x, game.powerUps[a].pos.y, 5, 5, hdcPowerup2, 0, 0, bmEnemyBullet.bmWidth, bmEnemyBullet.bmHeight, SRCCOPY);
					if (_tcscmp(game.powerUps[a].type, TEXT("ICE")) == 0)
						StretchBlt(auxDC, game.powerUps[a].pos.x, game.powerUps[a].pos.y, 5, 5, hdcPowerup3, 0, 0, bmEnemyBullet.bmWidth, bmEnemyBullet.bmHeight, SRCCOPY);
					if (_tcscmp(game.powerUps[a].type, TEXT("BATTERY")) == 0)
						StretchBlt(auxDC, game.powerUps[a].pos.x, game.powerUps[a].pos.y, 5, 5, hdcPowerup4, 0, 0, bmEnemyBullet.bmWidth, bmEnemyBullet.bmHeight, SRCCOPY);
					if (_tcscmp(game.powerUps[a].type, TEXT("ALCOOL")) == 0)
						StretchBlt(auxDC, game.powerUps[a].pos.x, game.powerUps[a].pos.y, 5, 5, hdcPowerup5, 0, 0, bmEnemyBullet.bmWidth, bmEnemyBullet.bmHeight, SRCCOPY);
					if (_tcscmp(game.powerUps[a].type, TEXT("LIFE")) == 0)
						StretchBlt(auxDC, game.powerUps[a].pos.x, game.powerUps[a].pos.y, 5, 5, hdcPowerup6, 0, 0, bmEnemyBullet.bmWidth, bmEnemyBullet.bmHeight, SRCCOPY);
				}
				;
				
			}

			

				
			

            HDC hdc = BeginPaint(hWnd, &ps);

			BitBlt(hdc, 0, 0, nX, nY, auxDC, 0, 0, SRCCOPY);

			EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);

		DeleteObject(hNave);
		DeleteDC(hdcNave);
		DeleteObject(bg);
		DeleteObject(auxBM);
		DeleteDC(auxDC);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Manipulador de mensagem para caixa 'Sobre'.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam){
    UNREFERENCED_PARAMETER(lParam);

    switch (message){

    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Login(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	TCHAR buffer[20];

	switch (message) {

	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDOK:
				if (GetDlgItemText(hDlg, IDC_LOGIN, (LPTSTR)buffer, 20)) {
					_tcscpy_s(request.name, buffer);
					_tcscpy_s(request.arrayMessages[0], TEXT("Begin"));
					EndDialog(hDlg, LOWORD(wParam));

					ZeroMemory(&Ov, sizeof(Ov));
					ResetEvent(IOReady);
					Ov.hEvent = IOReady;

					WriteFile(hPipe, &request, sizeof(request), &d, &Ov);
					WaitForSingleObject(IOReady, INFINITE);
					GetOverlappedResult(hPipe, &Ov, &d, FALSE);
					return true;

				}
				break;
			case IDCANCEL:
				EndDialog(hDlg, LOWORD(wParam));
				exit(-1);
				return true;
				break;

			default:
				return true;
		}
	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		return (INT_PTR)TRUE;

	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK End(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);

	switch (message) {

	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			EndDialog(hDlg, LOWORD(wParam));
			_tcscpy_s(request.arrayMessages[0], TEXT("Exit"));
		
			ZeroMemory(&Ov, sizeof(Ov));
			ResetEvent(IOReady);
			Ov.hEvent = IOReady;

			WriteFile(hPipe, &request, sizeof(request), &d, &Ov);
			WaitForSingleObject(IOReady, INFINITE);
			GetOverlappedResult(hPipe, &Ov, &d, FALSE);

			InvalidateRect(hWnd, NULL, FALSE);

			DestroyWindow(hWnd);
			exit(-1);
			return (INT_PTR)TRUE;
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		_tcscpy_s(request.arrayMessages[0], TEXT("Exit"));
		
		ZeroMemory(&Ov, sizeof(Ov));
		ResetEvent(IOReady);
		Ov.hEvent = IOReady;

		WriteFile(hPipe, &request, sizeof(request), &d, &Ov);
		WaitForSingleObject(IOReady, INFINITE);
		GetOverlappedResult(hPipe, &Ov, &d, FALSE);

		InvalidateRect(hWnd, NULL, FALSE);

		DestroyWindow(hWnd);
		exit(-1);
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

DWORD WINAPI receiveGame(LPVOID param) {

	DWORD d;

	//HANDLE IOReady;
	//OVERLAPPED Ov;

	//IOReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	while (1) {
		ZeroMemory(&Ov, sizeof(Ov));
		ResetEvent(IOReady);
		Ov.hEvent = IOReady;

		ReadFile(hPipe, &game, sizeof(Game), &d, &Ov);
		WaitForSingleObject(IOReady, INFINITE);
		GetOverlappedResult(hPipe, &Ov, &d, FALSE);
		
		for (int i = 0; i < 3; i++) {
			if (_tcscmp(game.players[i].username, request.name) == 0) {
				if (game.players[i].lives <= 0) {
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ENDDIALOG), hWnd, End);
				}
			}
		}
		

		InvalidateRect(hWnd, NULL, FALSE);
	}

	return 0;
}
