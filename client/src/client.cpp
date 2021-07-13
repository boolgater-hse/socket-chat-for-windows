#define HAVE_STRUCT_TIMESPEC
#define _CRT_SECURE_NO_WARNINGS

#include <pthread.h>
#pragma comment(lib, "ws2_32.lib")

#include <winsock.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void* ReceiveFromServer(void* param)
{
    int bytes;
    SOCKET server = (SOCKET)param;
    char* receive = (char*)calloc(1024, sizeof(char));
    while (1)
    {
        bytes = recv(server, receive, 1024, 0);
        if (bytes > 0)
        {
            receive[bytes] = 0;
            printf("%s\n", receive);
        }
    }
    return (void*)0;
}

void StartChatting(const char* ip, int port)
{
    SOCKET client;
    client = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (client == INVALID_SOCKET)
    {
        printf("Error create socket\n");
        return;
    }
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port); //the same as in server
    server.sin_addr.S_un.S_addr = inet_addr(ip); //special look-up address
    if (connect(client, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        printf("Can't connect to server\n");
        closesocket(client);
        return;
    }

    char* transmit = (char*)calloc(1024, sizeof(char));
    char* receive = (char*)calloc(1024, sizeof(char));

    int bytes = recv(client, receive, 1024, 0);
    if (bytes > 0)
        receive[bytes] = 0;
    printf("%s\n", receive);

    while (1)
    {
        bytes = recv(client, receive, 1024, 0);
        receive[bytes] = 0;

        if (strcmp(receive, "Welcome to the main chat!\n") == 0)
        {
            system("cls");
            printf("%s\n", receive);
            break;
        }

        if (strcmp(receive, "Wrong password. Try again!") == 0)
        {
            system("cls");
            printf("%s\n", receive);
            continue;
        }

        printf("%s", receive);
        fgets(transmit, 256, stdin);
        if (transmit[strlen(transmit) - 1] == '\n')
            transmit[strlen(transmit) - 1] = 0;

        send(client, transmit, strlen(transmit), 0);
        fflush(stdin);
    }

    pthread_t mythread;
    int status = pthread_create(&mythread, NULL, ReceiveFromServer, (void*)client);
    pthread_detach(mythread);

    while (1)
    {
        fgets(transmit, 1024, stdin);

        if (strstr(transmit, "/create ") || strstr(transmit, "/cr ") ||
            strstr(transmit, "/go "))
        {
            system("cls");
        }

        send(client, transmit, strlen(transmit), 0);

        if (strcmp(transmit, "/quit\n") == 0 || strcmp(transmit, "/q\n") == 0)
        {
            exit(0);
        }

        fflush(stdin);
        Sleep(500);
    }

    closesocket(client);
}

int main(int argc, char* argv[])
{
    WSADATA wsd;
    if (WSAStartup(MAKEWORD(1, 1), &wsd) != 0)
    {
        printf("Can't connect to socket lib");
        return 1;
    }

    if (argc == 1)
    {
        StartChatting("127.0.0.1", 1111);
    }
    else
    {
        StartChatting(argv[1], atoi(argv[2]));
    }

    return 0;
}
