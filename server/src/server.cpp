#define _CRT_SECURE_NO_WARNINGS
#define HAVE_STRUCT_TIMESPEC

#include <pthread.h>
#pragma comment(lib, "ws2_32.lib")

#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <time.h>

#include "funcs.h"

#define MAX_CLIENTS 10

pthread_mutex_t mutex;
pthread_mutex_t mutex_file;

CLIENT clients[MAX_CLIENTS];
CHAT rooms[MAX_CLIENTS];
int clientsNum = 0;
int roomsNum = 0;

int FindClient(int id)
{
	for (int i = 0; i <= clientsNum; i++)
	{
		if (clients[i].id == id)
			return i;
	}
	return -1;
}

void CreateRoom(const char* name, int client)
{
	strcpy(rooms[roomsNum].name, name);
	sprintf(rooms[roomsNum].filename, "data/chats/%s.txt", name);
	if (client > -1)
	{
		strcpy(rooms[roomsNum].admin, clients[client].name);
	}
	else
	{
		strcpy(rooms[roomsNum].admin, "\0");
	}
	roomsNum++;
}

int SignIn(SOCKET client)
{
	int bytes = 0;
	char* login = (char*)calloc(256, sizeof(char));
	char* password = (char*)calloc(256, sizeof(char));

	send(client, "WELCOME TO THE SERVER!\n", strlen("WELCOME TO THE SERVER!\n"), 0);
	int flag;

	while (1)
	{
		int flag = 0;
		send(client, "Enter your login: ", strlen("Enter your login: "), 0);
		bytes = recv(client, login, 256, 0);
		login[bytes] = 0;

		send(client, "Enter your password: ", strlen("Enter your password: "), 0);
		bytes = recv(client, password, 256, 0);
		password[bytes] = 0;

		if (bytes > 0)
		{
			int rows;
			char** base = ReadData("data/clientbase.txt", &rows);
			char* login_format = (char*)calloc(strlen(login) + 10, sizeof(char));
			char* password_format = (char*)calloc(strlen(login) + 10, sizeof(char));

			sprintf(login_format, "login:%s\n", login);
			sprintf(password_format, "password:%s\n", password);

			for (int i = 0; i < rows; i++)
			{
				if (strcmp(base[i], login_format) == 0)
				{
					if (strcmp(base[i + 1], password_format) == 0)
					{
						clients[clientsNum].id = clientsNum;
						int index = FindClient(clientsNum);
						strcpy(clients[index].name, login);
						clients[index].status = 1;
						clients[index].socket = client;
						clients[index].curChat = 0;
						clientsNum++;
						send(client, "Welcome to the main chat!\n", strlen("Welcome to the main chat!\n"), 0);

						pthread_mutex_lock(&mutex);
						printf("%s logged in\n", clients[index].name);
						pthread_mutex_unlock(&mutex);

						return index;
					}
					else
					{
						flag = 1;
						send(client, "Wrong password. Try again!", strlen("Wrong password. Try again!"), 0);
						break;
					}
				}
			}

			if (flag < 1)
			{
				clients[clientsNum].id = rows - 1;
				clients[clientsNum].socket = client;
				clients[clientsNum].friendsNum = 0;
				clients[clientsNum].status = 1;
				clients[clientsNum].curChat = 0;
				strcpy(clients[clientsNum].name, login);
				clientsNum++;

				pthread_mutex_lock(&mutex);
				printf("%s registered\n", clients[clientsNum - 1].name);
				pthread_mutex_unlock(&mutex);

				AddData(base, login_format, &rows);
				AddData(base, password_format, &rows);
				AddData(base, "\n", &rows);

				pthread_mutex_lock(&mutex_file);
				WriteData("data/clientbase.txt", rows, base);
				pthread_mutex_unlock(&mutex_file);

				send(client, "Welcome to the main chat!\n", strlen("Welcome to the main chat!\n"), 0);

				return (clientsNum - 1);
			}
		}
	}
}

void* ClientService(void* param)
{
	SOCKET client = (SOCKET)param;

	int index = SignIn(client);

	char* receive = (char*)calloc(1024, sizeof(char));
	char* transmit = (char*)calloc(1024, sizeof(char));
	char* date = (char*)calloc(50, sizeof(char));
	int bytes;

	sprintf(transmit, "%s joined\n", clients[index].name);
	for (int i = 0; i < clientsNum; i++)
		if (i != index && clients[i].curChat == clients[index].curChat && clients[i].status == 1)
			send(clients[i].socket, transmit, strlen(transmit), 0);

	char* hist = ReadHistory(rooms[clients[index].curChat].filename);
	send(client, hist, strlen(hist), 0);

	while (1)
	{
		bytes = recv(client, receive, 1024, 0);

		if (bytes > 0)
		{
			receive[bytes] = 0;

			if (strcmp(receive, "/help\n") == 0)
			{
				char* help = ReadHistory("data/help.txt");
				send(client, help, strlen(help), 0);
				continue;
			}

			if (strstr(receive, "/friend ") || (strstr(receive, "/f ")) || 
				strstr(receive, "/remove ") || strstr(receive, "/rm ") || 
				strstr(receive, "/invite ") || strstr(receive, "/inv "))
			{
				char* nickname = (char*)calloc(30, sizeof(char));
				int k = 0, j = 0;
				while (receive[k] != ' ')
					k++;
				k++;
				while (receive[k] != '\n')
				{
					nickname[j++] = receive[k];
					k++;
				}

				if (strstr(receive, "/friend ") || strstr(receive, "/f "))
				{
					for (int i = 0; i < clientsNum; i++)
						if (strcmp(clients[i].name, nickname) == 0)
						{
							clients[index].friendsList[clients[index].friendsNum] = i;
							clients[index].friendsNum++;

							sprintf(transmit, "%s follows you. ", clients[index].name);

							for (int j = 0; j < clients[i].friendsNum; j++)
							{
								if (clients[clients[i].friendsList[j]].name == clients[index].name)
								{
									strcat(transmit, "You are friends now!");
									break;
								}
							}

							strcat(transmit, "\n");
							send(clients[i].socket, transmit, strlen(transmit), 0);
							break;
						}
				}
				else if (strstr(receive, "/remove ") || strstr(receive, "/rm "))
				{
					for (int i = 0; i < clients[index].friendsNum; i++)
						if (strcmp(clients[clients[index].friendsList[i]].name, nickname) == 0)
						{
							int friendIndex = clients[index].friendsList[i];
							if (i != clients[index].friendsNum - 1)
							{
								for (int j = i + 1; j < clients[index].friendsNum; j++)
									clients[index].friendsList[j - 1] = clients[index].friendsList[j];
							}

							clients[index].friendsNum--;
							sprintf(transmit, "%s unfollows you. You are not friends anymore!", clients[index].name);
							send(clients[friendIndex].socket, transmit, strlen(transmit), 0);
							break;
						}
				}
				else
				{
					int friendIndex;
					for (int i = 0; i < clients[index].friendsNum; i++)
						if (strcmp(clients[clients[index].friendsList[i]].name, nickname) == 0)
						{
							friendIndex = clients[index].friendsList[i];

							int flag = 0;
							for (int j = 0; j < clients[friendIndex].chatsNum; j++)
								if (clients[friendIndex].availableChats[i] == clients[index].curChat)
								{
									flag = 1;
									break;
								}

							if (flag > 0)
								send(client, "Your friend is already here!\n", strlen("Your friend is already here!\n"), 0);
							else
							{
								sprintf(transmit, "You have been invited in %s room!\n", rooms[clients[index].curChat].name);
								send(clients[friendIndex].socket, transmit, strlen(transmit), 0);
								clients[friendIndex].availableChats[clients[friendIndex].chatsNum] = clients[index].curChat;
								clients[friendIndex].chatsNum++;
							}

							break;
						}
				}



				continue;
			}

			if (strcmp(receive, "/friendlist\n") == 0 || strcmp(receive, "/fl\n") == 0)
			{
				char* friendlist = (char*)calloc(1024, sizeof(char));
				friendlist[0] = '\n';
				for (int i = 0; i < clients[index].friendsNum; i++)
				{
					CLIENT* tmp = &(clients[clients[index].friendsList[i]]);
					strcat(friendlist, tmp->name);
					if (tmp->status > 0)
						strcat(friendlist, " - online\n");
					else
						strcat(friendlist, " - offline\n");
				}
				send(client, friendlist, strlen(friendlist), 0);
				continue;
			}

			if (strstr(receive, "/create ") || strstr(receive, "/cr ") ||
				strstr(receive, "/go "))
			{
				char* roomName = (char*)calloc(30, sizeof(char));
				int k = 0, j = 0;
				while (receive[k] != ' ')
					k++;
				k++;
				while (receive[k] != '\n')
				{
					roomName[j++] = receive[k];
					k++;
				}

				if (strstr(receive, "/create ") || strstr(receive, "/cr "))
				{
					send(client, "You created room!\n", strlen("You created room!\n"), 0);

					CreateRoom(roomName, index);

					clients[index].availableChats[clients[index].chatsNum] = roomsNum - 1;
					clients[index].chatsNum++;
					clients[index].curChat = roomsNum - 1;

					int roomIndex;
					for (int i = 0; i < clients[index].chatsNum; i++)
						if (strcmp(rooms[clients[index].availableChats[i]].name, roomName) == 0)
						{
							roomIndex = clients[index].availableChats[i];
							break;
						}

					clients[index].curChat = roomIndex;

					char* roomHist = ReadHistory(rooms[roomIndex].filename);
					send(client, roomHist, strlen(roomHist), 0);
				}
				else
				{
					int roomIndex;
					for (int i = 0; i < clients[index].chatsNum; i++)
						if (strcmp(rooms[clients[index].availableChats[i]].name, roomName) == 0)
						{
							roomIndex = clients[index].availableChats[i];
							break;
						}

					clients[index].curChat = roomIndex;

					sprintf(transmit, "Welcome to the %s chat!\n", rooms[roomIndex].name);
					send(client, transmit, strlen(transmit), 0);

					char* roomHist = ReadHistory(rooms[roomIndex].filename);
					send(client, roomHist, strlen(roomHist), 0);

					sprintf(transmit, "%s joined\n", clients[index].name);
					for (int i = 0; i < clientsNum; i++)
						if (i != index && clients[i].curChat == clients[index].curChat && clients[i].status == 1)
							send(clients[i].socket, transmit, strlen(transmit), 0);
				}

				continue;
			}

			if (strcmp(receive, "/roomlist\n") == 0 || strcmp(receive, "/rl\n") == 0)
			{
				char* roomlist = (char*)calloc(1024, sizeof(char));
				roomlist[0] = '\n';
				for (int i = 0; i < clients[index].chatsNum; i++)
				{
					CHAT* tmp = &(rooms[clients[index].availableChats[i]]);
					sprintf(transmit, "%s\n", tmp->name);
					strcat(roomlist, transmit);
				}
				send(client, roomlist, strlen(roomlist), 0);
				continue;
			}

			if (strcmp(receive, "/quit\n") == 0 || strcmp(receive, "/q\n") == 0)
			{
				clients[index].status = 0;
				sprintf(transmit, "%s goes offline", clients[index].name);

				pthread_mutex_lock(&mutex);
				printf("%s logged out\n", clients[index].name);
				pthread_mutex_unlock(&mutex);

				for (int i = 0; i < clientsNum; i++)
					if (i != index && clients[i].curChat == clients[index].curChat && clients[i].status == 1)
						send(clients[i].socket, transmit, strlen(transmit), 0);

				continue;
			}

			if (strstr(receive, "/"))
			{
				send(client, "Unknown command. Type '/help' for help.\n", strlen("Unknown command. Type '/help' for help.\n"), 0);

				continue;
			}

			int size;
			char** history = ReadData(rooms[clients[index].curChat].filename, &size);

			time_t seconds = time(NULL);
			tm* info = localtime(&seconds);

			sprintf(date, "Delivered on (%02d:%02d, %02d.%02d.%02d)\n", info->tm_hour, info->tm_min,
				info->tm_mday, info->tm_mon + 1, info->tm_year + 1900);

			sprintf(transmit, "%s: %s%s", clients[index].name, receive, date);
			AddData(history, transmit, &size);
			AddData(history, "\n", &size);

			pthread_mutex_lock(&mutex_file);
			WriteData(rooms[clients[index].curChat].filename, size, history);
			pthread_mutex_unlock(&mutex_file);

			for (int i = 0; i < clientsNum; i++)
			{
				if (i != index && clients[i].curChat == clients[index].curChat && clients[i].status == 1)
					send(clients[i].socket, transmit, strlen(transmit), 0);
				if (i == index)
					send(clients[index].socket, date, strlen(date), 0);
			}
		}
	}

	return (void*)0;
}

int CreateServer(int port)
{
	SOCKET server, client;
	sockaddr_in localaddr, clientaddr;
	int size;
	server = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (server == INVALID_SOCKET)
	{
		printf("Error create server\n");
		return 1;
	}
	localaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(port); // port number is for example, must be more than 1024
	if (bind(server, (sockaddr*)&localaddr, sizeof(localaddr)) == SOCKET_ERROR)
	{
		printf("Can't start server\n");
		return 2;
	}
	else
	{
		printf("Server is started\n");
	}

	listen(server, MAX_CLIENTS);
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutex_file, NULL);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		clients[i].id = -1;
		clients[i].curChat = -1;
		clients[i].friendsList = (int*)calloc(MAX_CLIENTS, sizeof(int));
		clients[i].name = (char*)calloc(30, sizeof(char));
		clients[i].friendsNum = 0;
		clients[i].availableChats = (int*)calloc(MAX_CLIENTS, sizeof(int));
		clients[i].chatsNum = 1;

		rooms[i].name = (char*)calloc(30, sizeof(char));
		rooms[i].admin = (char*)calloc(30, sizeof(char));
		rooms[i].filename = (char*)calloc(30, sizeof(char));
	}

	CreateRoom("main", -1);

	while (1)
	{
		size = sizeof(clientaddr);
		client = accept(server, (sockaddr*)&clientaddr, &size);

		if (client == INVALID_SOCKET)
		{
			printf("Error accept client\n");
			continue;
		}
		else
		{
			printf("Client is accepted\n");
		}
		pthread_t mythread;
		int status = pthread_create(&mythread, NULL, ClientService, (void*)client);
		pthread_detach(mythread);
	}
	pthread_mutex_destroy(&mutex_file);
	pthread_mutex_destroy(&mutex);
	printf("Server is stopped\n");
	closesocket(server);
	return 0;
}

int main(int argc, char* argv[])
{
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(1, 1), &wsd) == 0)
	{
		printf("Connected to socket lib\n");
	}
	else
	{
		return 1;
	}

	if (argc == 1)
	{
		return CreateServer(1111);
	}
	else
	{
		return CreateServer(atoi(argv[1]));
	}
}
