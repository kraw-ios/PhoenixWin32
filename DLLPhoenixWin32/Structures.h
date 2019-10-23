#pragma once
//Marco Duarte & Marco Lopes 11/04/2018

#define T_STRING 30
#define TAM 600

#include <Windows.h>
#include <tchar.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>
#include <time.h>


typedef struct {
	int x;
	int y;
}Position;

typedef struct {
	Position pos;
	bool usage = 0;
}Bullets;

typedef struct {
	TCHAR username[T_STRING] = TEXT("\0");
	TCHAR powerup[T_STRING];
	int lives;
	int dimX;
	int dimY;
	int n_bullets;
	int imune = 0;
	int inverted = 0;
	int shootingRate;
	int score;
	Position pos;
	Bullets bullet[10];
}Player;

typedef struct {
	TCHAR type[T_STRING];
	int dimX;
	int dimY;
	int usage = 0;
	Position pos;
}Powerups;

typedef struct {
	TCHAR type[T_STRING];
	int id;
	int dimX;
	int dimY;
	int velocity;
	int shootingRate;
	int life;
	int num_bullet;
	int ative;
	Position pos;
	Bullets bullet[5];
}Enemies;


typedef struct {
	int level;
	int score;
	int numberEnem;
	int number_powerups;
	int numberPlayers;
	Player players[3];
	Powerups powerUps[5];
	Enemies enemies[10];
	TCHAR map[TAM][TAM];
}Game;
