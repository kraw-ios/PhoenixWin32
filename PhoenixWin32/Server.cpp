/*-------------------------------------------
Marco Duarte & Marco Lopes 11/04/2018
-------------------------------------------*/

#include "../DLLPhoenixWin32/DLLPhoenixWin32.h"
#include "../DLLPhoenixWin32/Structures.h"


DWORD WINAPI manageEnemies(LPVOID param);
void serverMenu();
void createMonsters(int lvl);


HANDLE hMutexC;
HANDLE hMemory;
HANDLE hSemS; // pode escrever
HANDLE hSemC; // pode ler
HANDLE hMutexBullet;
HANDLE hMutexPowerupPlayer;
HANDLE hMutexMovePlayer;
HANDLE hMutexBulletPlayer;
HANDLE hMutexPowerup;

int stopmove = 0;

Game *game;

int _tmain(int argc, LPTSTR argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	srand((unsigned int)time(NULL));

	hSemS = CreateSemaphore(NULL, 10, 10, ServerSemaphoreName);
	hSemC = CreateSemaphore(NULL, 0, 10, ClientSemaphoreName);

	hMutexC = CreateMutex(NULL, FALSE, ServerMutexName);

	hMutexPowerup = CreateMutex(NULL, FALSE, TEXT("PowerupMutex"));
	hMutexPowerupPlayer = CreateMutex(NULL, FALSE, TEXT("PowerupPlayerMutex"));
	hMutexMovePlayer = CreateMutex(NULL, FALSE, TEXT("MovePlayerMutex"));
	hMutexBulletPlayer = CreateMutex(NULL, FALSE, TEXT("BulletPlayerMutex"));
	hMutexBullet = CreateMutex(NULL, FALSE, TEXT("BulletMutex"));

	hMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Game), SharedMemoryName);

	game = (Game *)MapViewOfFile(hMemory, FILE_MAP_WRITE, 0, 0, sizeof(Game));


	
	

	game->level = 1;
	game->numberPlayers = 0;


	


	if (game == NULL) {
		_tprintf(TEXT("Error creating the Map view of file!\n"));
		return -1;
	}



	serverMenu();


	CloseHandle(hMutexC);

	UnmapViewOfFile(game);
}


int random_l_h(int min, int max) {
	return (rand() % (max + min));
}

DWORD WINAPI checkEnemiesAndPlayers(LPVOID param) {
	int i, j;

	while (1) {
		for (i = 0; i < game->numberEnem; i++) {
			for (j = 0; j < game->numberPlayers; j++) {
				if (((game->enemies[i].pos.x) >= (game->players[j].pos.x) && (game->enemies[i].pos.x) <= (game->players[j].pos.x + game->players[j].dimX)
					|| (game->enemies[i].pos.x + game->enemies[i].dimX) >= (game->players[j].pos.x) && (game->enemies[i].pos.x + game->enemies[i].dimX) <= (game->players[j].pos.x + game->players[j].dimX))
					&& ((game->enemies[i].pos.y) >= (game->players[j].pos.y) && (game->enemies[i].pos.y) <= (game->players[j].pos.y + game->players[j].dimY)
					|| (game->enemies[i].pos.y + game->enemies[i].dimY) >= (game->players[j].pos.y) && (game->enemies[i].pos.y + game->enemies[i].dimY) <= (game->players[j].pos.y + game->players[j].dimY))) {
					

					game->enemies[i].ative = 0;
					for (int k = i; k < game->numberEnem; i++) {
						game->enemies[k] = game->enemies[k + 1];
					}


					game->players[j].lives = 0;

					game->numberEnem--;
					game->numberPlayers--;
					if (game->numberEnem == 0) {
						createMonsters(game->level++);
					}
				}
					
			}
		}

	}
	return 0;
}

DWORD WINAPI managePlayerBullets(LPVOID param) {
	Player *player = (Player *)param;
	int x = 0;

	while (player->bullet[x].usage != 0)
		x++;

	if (x <= 10) {
		player->n_bullets++;
		player->bullet[x].usage = 1;
	}

	player->bullet[x].pos.x = player->pos.x;
	player->bullet[x].pos.y = player->pos.y;


	do {
		WaitForSingleObject(hMutexBulletPlayer, INFINITE);
		player->bullet[x].pos.y -= 15;
		
		//verifica se apanhou algum inimigo
		for (int a = 0; a < 10; a++) {
			if (player->bullet[x].pos.y >= game->enemies[a].pos.y && player->bullet[x].pos.y <= (game->enemies[a].pos.y + game->enemies[a].dimY)) {
				if (player->bullet[x].pos.x >= game->enemies[a].pos.x && player->bullet[x].pos.x <= (game->enemies[a].pos.x + game->enemies[a].dimX)) {
					//FALTA VER AS VIDAS DELES E MORRER
					game->enemies[a].life--;
					if (game->enemies[a].life <= 0) {
						game->enemies[a].ative = 0;
						for (int j = a; j < game->numberEnem; j++) {
							game->enemies[j] = game->enemies[j + 1];
						}
						game->numberEnem--;
						player->score++;
						if (game->numberEnem == 0){
							createMonsters(game->level++);
						}
					}
					player->n_bullets--;
					player->bullet[x].usage = 0;
					return 0;
				}
			}
		}
		ReleaseMutex(hMutexBulletPlayer);
		Sleep(player->shootingRate);
	} while ((player->bullet[x].pos.y - 15) >= 0);

	player->n_bullets--;
	player->bullet[x].usage = 0;
	return 0;
}

boolean checkPlayerAllies(int currentPlayer, int posX) {

	int i;

	for (i = 0; i < game->numberPlayers; i++) {
		if (i != currentPlayer) {
			if ((game->players[currentPlayer].pos.x + posX) >= (game->players[i].pos.x) && (game->players[currentPlayer].pos.x + posX) <= (game->players[i].pos.x + game->players[i].dimX)
				|| (game->players[currentPlayer].pos.x + game->players[currentPlayer].dimX + posX) >= (game->players[i].pos.x) && (game->players[currentPlayer].pos.x + game->players[currentPlayer].dimX + posX) <= (game->players[i].pos.x + game->players[i].dimX))
				return false;
		}
	}

	return true;
}

void controlPlayer(SharedMessage *received, DWORD *posR) {
	
	DWORD *tID;
	int count = 0;
	tID = (DWORD *)malloc(sizeof(DWORD));

	if (_tcscmp(received->arrayMessages[*posR], TEXT("Begin")) == 0) {
		for (int i = 0; i < 3; i++) {
			if (_tcscmp(game->players[i].username, TEXT("\0")) == 0) {
				_tcscpy_s(game->players[i].username, received->name);
				_tprintf(TEXT("%s %s"), received->name, game->players[i].username);
				game->numberPlayers++;
				game->players[i].pos.x = 200; // falta fazer random
				game->players[i].pos.y = 550;
				game->players[i].lives = 2;
				game->players[i].dimX = 30;
				game->players[i].dimY = 50;
				game->players[i].n_bullets = 0;
				game->players[i].score = 0;
				game->players[i].shootingRate = 500;
				break;
			}

		}

	}
	else {
		for (int i = 0; i < 3; i++) {
			if (_tcscmp(game->players[i].username, received->name) == 0) {
				if (_tcscmp(received->arrayMessages[*posR], TEXT("Left")) == 0) {
					if (game->players[i].inverted == 0) {
						WaitForSingleObject(hMutexMovePlayer, INFINITE);
						if ((game->players[i].pos.x - 8) >= 0) {
							game->players[i].pos.x -= 8;
						}
						ReleaseMutex(hMutexMovePlayer);
					}
					if (game->players[i].inverted == 1) {
						WaitForSingleObject(hMutexMovePlayer, INFINITE);
						if ((game->players[i].pos.x + 8) <= TAM) {
							game->players[i].pos.x += 8;
						}
						ReleaseMutex(hMutexMovePlayer);
					}

				}
				if (_tcscmp(received->arrayMessages[*posR], TEXT("Right")) == 0) {
					if (game->players[i].inverted == 0) {
						WaitForSingleObject(hMutexMovePlayer, INFINITE);
						if ((game->players[i].pos.x + game->players[i].dimX + 8) <= TAM)
							game->players[i].pos.x += 8;
						ReleaseMutex(hMutexMovePlayer);
					}
					if (game->players[i].inverted == 1) {
						WaitForSingleObject(hMutexMovePlayer, INFINITE);
						if ((game->players[i].pos.x - 8) >= 0)
							game->players[i].pos.x -= 8;
						ReleaseMutex(hMutexMovePlayer);
					}

				}
				if (_tcscmp(received->arrayMessages[*posR], TEXT("Fire")) == 0) {
					if (game->players[i].n_bullets <= 10)
						CreateThread(NULL, 0, managePlayerBullets, &game->players[i], 0, tID);
				}
				_tprintf(TEXT("%s %s : %d"), received->name, game->players[i].username, game->players[i].pos.x);
			}

		}
	}
	return;
}

unsigned int _stdcall listenerMsgFromGateway(void *p) {
	ContrData * data = (ContrData *)p;

	SharedMessage received;
	unsigned int current = peekMessage(data);
	DWORD *tID, posR;

	tID = (DWORD *)malloc(sizeof(DWORD));

	while (data->threadShouldContinue) {
		Sleep(200);
		if (current < peekMessage(data)) {
			
			posR = readMessage(data, &received);
			current = received.numberOfMessages;
			_tprintf(TEXT("[%d] %s: %s \n"), current, received.name, received.arrayMessages[posR]);
			if (_tcscmp(received.arrayMessages[posR], TEXT("ACABAR")) == 0) {
				data->threadShouldContinue = 0;
			}
			else {
				controlPlayer(&received, &posR);
	
			}

		}
		
	}

	CloseHandle(data->hMemoryMsg);
	return 0;
}

boolean checkAllies(int currentEnemy, Position pos) {
	
	int i;

	for (i = 0; i < game->numberEnem; i++) {
		if (i != currentEnemy) {
			if (((game->enemies[currentEnemy].pos.x + pos.x) >= (game->enemies[i].pos.x) && (game->enemies[currentEnemy].pos.x + pos.x) <= (game->enemies[i].pos.x + game->enemies[i].dimX)
				|| (game->enemies[currentEnemy].pos.x + game->enemies[currentEnemy].dimX + pos.x) >= (game->enemies[i].pos.x) && (game->enemies[currentEnemy].pos.x + game->enemies[currentEnemy].dimX + pos.x) <= (game->enemies[i].pos.x + game->enemies[i].dimX))
				&& ((game->enemies[currentEnemy].pos.y + pos.y) >= (game->enemies[i].pos.y) && (game->enemies[currentEnemy].pos.y + pos.y) <= (game->enemies[i].pos.y + game->enemies[i].dimY)
				|| (game->enemies[currentEnemy].pos.y + game->enemies[currentEnemy].dimY + pos.y) >= (game->enemies[i].pos.y) && (game->enemies[currentEnemy].pos.y + game->enemies[currentEnemy].dimY + pos.y) <= (game->enemies[i].pos.y + game->enemies[i].dimY)))
				return false;
		}
	}
	
	return true;
}

DWORD WINAPI dropBombs(LPVOID param) {
	Enemies *enemy = (Enemies*)param;
	int x = 0;

	while (enemy->bullet[x].usage != 0)
		x++;

	if (x < 5) {
		enemy->num_bullet++;
		enemy->bullet[x].usage = 1;
	}
	

	enemy->bullet[x].pos.x = enemy->pos.x + enemy->dimX;
	enemy->bullet[x].pos.y = enemy->pos.y + enemy->dimY;

	
	do {
		WaitForSingleObject(hMutexBullet, INFINITE);
		enemy->bullet[x].pos.y += 15;
		_tprintf(TEXT("\nBullet %d pos: %d"), x, enemy->bullet[x].pos.y);
		for (int i = 0; i < game->numberPlayers; i++) {
			if (enemy->bullet[x].pos.y >= game->players[i].pos.y &&  enemy->bullet[x].pos.y <= (game->players[i].pos.y + game->players[i].dimY))
				if (enemy->bullet[x].pos.x >= game->players[i].pos.x && enemy->bullet[x].pos.x <= (game->players[i].pos.x + game->players[i].dimX))
					if (game->players[i].imune == 0) {
						game->players[i].lives--;
						////VERIFICAR AS VIDAS DO PLAYER E TIRA-LO
						//if (game->players[i].lives <= 0) {
						//	for (int j = i; j < game->numberPlayers; j++) {
						//		game->players[j] = game->players[j + 1];
						//	}
						//	game->numberPlayers--;
						//} 
						enemy->num_bullet--;
						enemy->bullet[x].usage = 0;
						return 0;
					} 
					else {
						enemy->num_bullet--;
						enemy->bullet[x].usage = 0;
						return 0;
					}
						
		}
		ReleaseMutex(hMutexBullet);
		Sleep(enemy->shootingRate);
		
	} while ((enemy->bullet[x].pos.y + 15) <= TAM);

	enemy->num_bullet--;
	enemy->bullet[x].usage = 0;
	return 0;
}

DWORD WINAPI countPowerupsPlayer(LPVOID param) {
	Player *player = (Player *)param;

	if (_tcscmp(player->powerup, TEXT("SHIELD")) == 0) {
		WaitForSingleObject(hMutexPowerupPlayer, INFINITE);
		player->imune = 1;
		Sleep(10000);
		player->imune = 0;
		ReleaseMutex(hMutexPowerupPlayer);
	}
	if (_tcscmp(player->powerup, TEXT("BATTERY")) == 0) {
		WaitForSingleObject(hMutexPowerupPlayer, INFINITE);
		player->shootingRate = 400;
		Sleep(10000);
		player->shootingRate = 700;
		ReleaseMutex(hMutexPowerupPlayer);
	}
	if (_tcscmp(player->powerup, TEXT("LIFE")) == 0) {
		WaitForSingleObject(hMutexPowerupPlayer, INFINITE);
		player->lives++;
		ReleaseMutex(hMutexPowerupPlayer);
	}
	if (_tcscmp(player->powerup, TEXT("ALCOOL")) == 0) {
		WaitForSingleObject(hMutexPowerupPlayer, INFINITE);
		player->inverted = 1;
		Sleep(10000);
		player->inverted = 0;
		ReleaseMutex(hMutexPowerupPlayer);
	}

	game->number_powerups--;

	return 0;
}

DWORD WINAPI dropPowerups(LPVOID param) {
	Powerups *powerup = (Powerups *)param;
	int drop;
	HANDLE hThreadCount;
	drop = random_l_h(0, 100);
	powerup->usage = 1;
	powerup->pos.y = 0;
	powerup->pos.x = random_l_h(0,600);

	if (drop < 25) {
		_tcscpy_s(powerup->type, TEXT("SHIELD"));
	}
	if (drop >= 25 && drop < 50) {
		_tcscpy_s(powerup->type, TEXT("ENEMIES_FASTER"));
	}
	if (drop >= 50 && drop < 65) {
		_tcscpy_s(powerup->type, TEXT("ICE"));
	}
	if (drop >= 65 && drop < 80) {
		_tcscpy_s(powerup->type, TEXT("BATTERY"));
	}
	if (drop >= 80 && drop < 95) {
		_tcscpy_s(powerup->type, TEXT("ALCOOL"));
	}
	if (drop >= 95) {
		_tcscpy_s(powerup->type, TEXT("LIFE"));
	}


	do {
		WaitForSingleObject(hMutexPowerup, INFINITE);
		powerup->pos.y+=8;

		
		for (int i = 0; i < 3; i++) {

			
			if((powerup->pos.y >= game->players[i].pos.y) && (powerup->pos.y <= (game->players[i].pos.y + game->players[i].dimY)))
				if ((powerup->pos.x >= game->players[i].pos.x) && (powerup->pos.x <= (game->players[i].pos.x + game->players[i].dimX))) {

					if (_tcscmp(powerup->type, TEXT("ENEMIES_FASTER")) == 0 || _tcscmp(powerup->type, TEXT("ICE")) == 0) {
						
						if (_tcscmp(powerup->type, TEXT("ICE")) == 0) {
							stopmove = 1;
							Sleep(5000);
							stopmove = 0;
							game->number_powerups--;
							powerup->usage = 0;
							return 0;
						}

						if (_tcscmp(powerup->type, TEXT("ENEMIES_FASTER")) == 0) {
							for (int j = 0; j < game->numberEnem; j++) {
								game->enemies[j].velocity = 500;
								Sleep(5000);
								game->enemies[j].velocity = 1000;
								game->number_powerups--;
								powerup->usage = 0;
								return 0;
							}
						}

					}
					else {
						_tcscpy_s(game->players[i].powerup,powerup->type);
						// chamar thread para temporizar a powerup
						hThreadCount = CreateThread(NULL, 0, countPowerupsPlayer, &game->players[i], 0, NULL);

						powerup->usage = 0;
						return 0;
					}
						
				}

		}
		
		ReleaseMutex(hMutexPowerup);
		Sleep(1000); 
	} while ((powerup->pos.y + powerup->dimY) <= TAM);

	game->number_powerups--;
	powerup->usage = 0;
	return 0;
}

DWORD WINAPI manageEnemies(LPVOID param) {
	Enemies *enem;
	int ch;
	int i = 0;
	DWORD *threadID;
	Position pos;

	enem = (Enemies*)param;

	threadID = (DWORD *)malloc(5 * sizeof(Bullets));

	
	do {
			if (_tcsncmp(enem->type, TEXT("BASIC"), 5) == 0) {

				do {

					if (random_l_h(0, 100) <= 25) {
						if (enem->num_bullet < 5) {
							CreateThread(NULL, 0, dropBombs, enem, 0, NULL);
						}
					}


					pos.x = 25;
					pos.y = 25;

					if ((enem->pos.x + enem->dimX) <= TAM && (enem->pos.y + enem->dimY) <= TAM) {

						if (enem->pos.x + enem->dimX + pos.x >= TAM) {
							if (checkAllies(enem->id, pos)) {

								if (stopmove == 0) {
									if (enem->ative == 1) {

										enem->pos.y += pos.y;

										do {
											if (stopmove == 0) {
												if (enem->ative == 1) {
													
													WaitForSingleObject(hMutexC, INFINITE);

													enem->pos.x -= pos.x;

													ReleaseMutex(hMutexC);
													Sleep(enem->velocity);
													
												}
												else
													return 0;
											}
										} while (enem->pos.x - enem->dimX - pos.x > 0);

										enem->pos.y += pos.y;
									}
									else
										return 0;
								}
							}
						}
						else {
							if (checkAllies(enem->id, pos)) {
								if (stopmove == 0) {
									if (enem->ative == 1) {
										
										WaitForSingleObject(hMutexC, INFINITE);
										enem->pos.x += pos.x;
										ReleaseMutex(hMutexC);
										
									}
								}
							}
						}
					}

					Sleep(enem->velocity);

				} while (enem->pos.x + enem->dimX + pos.x < TAM || enem->pos.y + enem->dimY + pos.y < TAM);
			}

			if (_tcsncmp(enem->type, TEXT("RANDOM"), 4) == 0) {

				

				do {

					if (random_l_h(0, 100) <= 20) {
						if (enem->num_bullet <= 5) {
							CreateThread(NULL, 0, dropBombs, enem, 0, NULL);
						}
					}
						

					ch = random_l_h(0, 3);
					pos.x = 0;
					pos.y = 0;

					if (ch == 0)
						pos.x-=25;
					if (ch == 1)
						pos.y-=25;
					if (ch == 2)
						pos.x+=25;
					if (ch == 3)
						pos.y+=25;

					if ((enem->pos.x - enem->dimX + pos.x >= 0) && (enem->pos.y - enem->dimY + pos.y >= 0) && (enem->pos.y + enem->dimY + pos.y < TAM) && (enem->pos.x + enem->dimX + pos.x < TAM)) {
						if (checkAllies(enem->id, pos)) {

							if (stopmove == 0) {
								if (enem->ative == 1) {
									
									WaitForSingleObject(hMutexC, INFINITE);
									enem->pos.x += pos.x;
									enem->pos.y += pos.y;
									ReleaseMutex(hMutexC);
									
								}
								else
									return 0;
							}
						}
					}

					Sleep(enem->velocity);
				} while (checkAllies(enem->id, pos) == false || enem->pos.x + enem->dimX + pos.x < TAM || enem->pos.y + enem->dimY + pos.y < TAM);


				

			}

			if (_tcsncmp(enem->type, TEXT("INVERTED"), 5) == 0) {


				do {


					pos.x = 25; // 2
					pos.y = 25; // 2

					if ((enem->pos.x + enem->dimX) <= TAM && (enem->pos.y + enem->dimY) <= TAM) {

						if (enem->pos.y + enem->dimY + pos.y >= TAM) {
							if (checkAllies(enem->id, pos)) {

								if (stopmove == 0) {
									if (enem->ative == 1) {
										enem->pos.x += pos.x;

										do {

											if (stopmove == 0) {
												if (enem->ative == 1) {
													
													WaitForSingleObject(hMutexC, INFINITE);

													enem->pos.y -= pos.y;

													ReleaseMutex(hMutexC);
													Sleep(enem->velocity);
													
												}
												else
													return 0;
											}
										} while (enem->pos.y - enem->dimY - pos.y > 0);

										enem->pos.x += pos.x;
									}
								}
								else
									return 0;
							}
						}
						else {
							if (checkAllies(enem->id, pos)) {

								if (stopmove == 0) {
									if (enem->ative == 1) {
										
										WaitForSingleObject(hMutexC, INFINITE);
										enem->pos.y += pos.y;
										ReleaseMutex(hMutexC);
										
									}
									else
										return 0;
								}
							}
						}
					}

					Sleep(enem->velocity);

				} while (enem->pos.x + enem->dimX + pos.x < TAM || enem->pos.y + enem->dimY + pos.y < TAM);

			}
		
		
	}while (1);


	return 0;
}

void createMonsters(int lvl) {
	
	switch (lvl){
	case 1:
		game->numberEnem = 4;
		for (int i = 0; i < game->numberEnem; i++) {
			game->enemies[i].pos.x = 100*i;
			game->enemies[i].pos.y = 50*i;
			game->enemies[i].dimX = 30;
			game->enemies[i].dimY = 30;
			if (i == 0 || i == 1) {
				_tcscpy_s(game->enemies[i].type, TEXT("BASIC"));
			}
			else {
				_tcscpy_s(game->enemies[i].type, TEXT("INVERTED"));
			}
			game->enemies[i].velocity = 600;
			game->enemies[i].shootingRate = 400;
			game->enemies[i].id = i;
			game->enemies[i].num_bullet = 0;
			game->enemies[i].life = 1;
			game->enemies[i].ative = 1;
		}
		break;
	case 2:
		
		game->numberEnem = 5;

		for (int i = 0; i < game->numberEnem; i++) {
			game->enemies[i].pos.x = 100 * i;
			game->enemies[i].pos.y = 50 * i;
			game->enemies[i].dimX = 30;
			game->enemies[i].dimY = 30;
			if (i == 0 || i == 1) {
				_tcscpy_s(game->enemies[i].type, TEXT("RANDOM"));
			}
			if(i == 2 || i == 3) {
				_tcscpy_s(game->enemies[i].type, TEXT("BASIC"));
			}
			if(i == 4) {
				_tcscpy_s(game->enemies[i].type, TEXT("INVERTED"));
			}
			game->enemies[i].velocity = 600;
			game->enemies[i].shootingRate = 300;
			game->enemies[i].id = i;
			game->enemies[i].num_bullet = 0;
			game->enemies[i].life = 2;
			game->enemies[i].ative = 1;
		}

		break;
	case 3:

		game->numberEnem = 6;

		for (int i = 0; i < game->numberEnem; i++) {
			game->enemies[i].pos.x = 100 * i;
			game->enemies[i].pos.y = 50 * i;
			game->enemies[i].dimX = 30;
			game->enemies[i].dimY = 30;
			if (i == 0 || i == 1) {
				_tcscpy_s(game->enemies[i].type, TEXT("RANDOM"));
			}
			if (i == 2 || i == 3) {
				_tcscpy_s(game->enemies[i].type, TEXT("BASIC"));
			}
			if (i == 4 || i == 5) {
				_tcscpy_s(game->enemies[i].type, TEXT("INVERTED"));
			}
			game->enemies[i].velocity = 500;
			game->enemies[i].shootingRate = 100;
			game->enemies[i].id = i;
			game->enemies[i].num_bullet = 0;
			game->enemies[i].life = 3;
			game->enemies[i].ative = 1;
		}



	default:
		break;
	}
}


void serverMenu() {

	int option = 0, velocity = 1000, shootingRate = 1000, powerupFrequency = 15;

	game->number_powerups = 0;

	DWORD *threadID;
	unsigned int tIDM;
	HANDLE *hT, hTM;
	ContrData data;

	threadID = (DWORD *)malloc(game->numberEnem * sizeof(DWORD));
	hT = (HANDLE *)malloc(game->numberEnem * sizeof(HANDLE));

	

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
			return;
		}
	}
	
	data.sharedMessages = (SharedMessage *)MapViewOfFile(data.hMemoryMsg, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMessage));

	if(data.sharedMessages == NULL){
		_tprintf(TEXT("\nError creating the shared memory view (error: %d)!\n"), GetLastError());
		CloseHandle(data.hMemoryMsg);
		return;
	}

	data.sharedMessages->hSemM = CreateSemaphore(NULL, 1, 1, MessagesSemaphoreName);

	while (1) {
		if (option == 0) {
			_tprintf(TEXT("\n__________________________________\n"));
			_tprintf(TEXT("\tPHOENIX WIN32\n"));
			_tprintf(TEXT("__________________________________\n"));
			_tprintf(TEXT("\n\tSERVER\n"));
			_tprintf(TEXT("1 - Start Server Room\n"));
			_tprintf(TEXT("2 - Game Settings\n"));
			_tprintf(TEXT("3 - Credits\n"));
			_tprintf(TEXT("4 - Exit\n"));
			wscanf_s(L"%d", &option);

		}
		if (option == 1) {
			//// PREPARAR OS MONSTROS!!!
			
			createMonsters(game->level);

			data.threadShouldContinue = 1;
			_beginthreadex(0, 0, listenerMsgFromGateway, &data, 0, &tIDM);
			hTM = OpenThread(THREAD_ALL_ACCESS, FALSE, tIDM);
			if (hTM == NULL)
				_tprintf(TEXT("ERROR CREATING MESSAGES THREAD"));

			for (int i = 0; i < game->numberEnem; i++) {

				hT[i] = CreateThread(NULL, 0, manageEnemies, &game->enemies[i], 0, &threadID[i]);
				
				if (hT[i] != NULL)
					_tprintf(TEXT("Thread created successfully! %d"), i);

			}
			
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)checkEnemiesAndPlayers, NULL, 0, NULL);


			while (1) {

				for (int i = 0; i < game->numberEnem; i++) {
					_tprintf(TEXT("\nEnemy %d - posX: %d posY: %d") ,i, game->enemies[i].pos.x, game->enemies[i].pos.y);
				}

				if (game->numberEnem <= 0) {
					game->level++;
					option = 1;
					continue;
				}


				if (random_l_h(0, 100) <= powerupFrequency) {
					if (game->number_powerups <= 5) {
					
						int x = 0;
						while (game->powerUps[x].usage != 0)
							x++;

						if (x <= 5) {
							game->number_powerups++;
							CreateThread(NULL, 0, dropPowerups, &game->powerUps[x], 0, NULL);
						}
					}
					
				}

				Sleep(game->enemies[0].velocity);
			}



			free(threadID);
			free(hT);
			
		}
		if (option == 2) {
			_tprintf(TEXT("\n\tGAME SETTINGS\n"));
			_tprintf(TEXT("1 - Object Settings\n"));
			_tprintf(TEXT("2 - Level Settings\n"));
			_tprintf(TEXT("3 - Return\n"));
			wscanf_s(L"%d", &option);

			if (option == 1) {
				_tprintf(TEXT("\n\tOBJECT SETTINGS\n"));
				_tprintf(TEXT("1 - Ship Velocity\n"));
				_tprintf(TEXT("2 - Shooting Rate\n"));
				_tprintf(TEXT("3 - Powerup Frequency\n"));
				_tprintf(TEXT("4 - Return\n"));
				wscanf_s(L"%d", &option);

				if (option == 1) {
					_tprintf(TEXT("Choose a Velocity for the Ships [miliseconds]: "));
					wscanf_s(L"%d", &velocity);
					for (int i = 0; i < game->numberEnem; i++) {
						game->enemies[i].velocity = velocity;
					}
				}

				if (option == 2) {
					_tprintf(TEXT("Choose the Shooting Rate for the Ships [miliseconds]: "));
					wscanf_s(L"%d", &shootingRate);
					for (int i = 0; i < game->numberEnem; i++) {
						game->enemies[i].shootingRate = shootingRate;
					}
				}

				if (option == 3) {
					_tprintf(TEXT("Choose the Frequency for the Powerups to begin [0-100%][Default: 15%] : "));
					wscanf_s(L"%d", &powerupFrequency);
				}

				if (option == 4) {
					option = 2;
					continue;
				}

			}

			if (option == 2) {
				_tprintf(TEXT("\n\tLEVEL SETTINGS\n"));
				_tprintf(TEXT("1 - Level 1\n"));
				_tprintf(TEXT("2 - Level 2\n"));
				_tprintf(TEXT("3 - Level 3\n"));
				wscanf_s(L"%d", &game->level);

				continue;
			}

			if (option == 3) {
				option = 0;
				continue;
			}
		}

		if (option == 3) {
			_tprintf(TEXT("\t### CREATORS ###\n"));
			_tprintf(TEXT("   Marco Duarte & Marco Lopes\n"));
			_tprintf(TEXT("        ISEC-DEIS - 2018\n"));
		}

		else {
			UnmapViewOfFile(data.sharedMessages);
			CloseHandle(data.hMemoryMsg);
			exit(EXIT_SUCCESS);
		}
	}
}