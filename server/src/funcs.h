#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock.h>
#define MAX_CLIENTS 10

typedef struct chat
{
	char* name;
	char* admin;
	char* filename;
} CHAT;

typedef struct client
{
	char* name;
	SOCKET socket;
	int id;
	int status;     // 1 - online, 0 - offline
	int curChat;
	int* availableChats;
	int chatsNum;
	int* friendsList;
	int friendsNum;

} CLIENT;

char** ReadData(const char* input_name, int* rows);
void AddData(char** data, const char* str, int* rows);
void WriteData(const char* output_name, int rows, char** data);
char* ReadHistory(const char* name);
void MakeFiles();