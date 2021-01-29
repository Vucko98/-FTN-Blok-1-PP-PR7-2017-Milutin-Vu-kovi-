#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include "conio.h"
#include<time.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma pack(1)

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 27016
#define BUFFER_SIZE 256
#define BUFFER_SIZE_REZULTAT 1024

DWORD WINAPI recvOdgovor(LPVOID lpParam);

int main()
{   
    srand(time(NULL));
    int iResult;
    SOCKET connectSocket = INVALID_SOCKET; 
    char dataBuffer[BUFFER_SIZE];    
        
    WSADATA wsaData;    
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }
    
    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;								// IPv4 protocol
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);	
    serverAddress.sin_port = htons(SERVER_PORT);

    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }
    
    DWORD clientID;
    HANDLE hRecv = CreateThread(NULL, 0, &recvOdgovor, &connectSocket, 0, &clientID);

    char offset;
    char i;
    int brZahteva = 1;
    char dimenzija;

    while (true)
    {
        dimenzija = (rand() % 5) + 1;

        offset = 0;
        _getch();
        dataBuffer[0] = 1; //oznaka klienta
        offset += sizeof(char);
        memcpy(dataBuffer + offset, &brZahteva, sizeof(int));
        offset += sizeof(int);        
        dataBuffer[offset] = dimenzija;
        offset += sizeof(char);
        printf("broj zahteva<%d> matrice dimenzije<%d>:", brZahteva++, dimenzija);
        for (i = 0; i < dimenzija * dimenzija; i++)
        {
            dataBuffer[offset] = rand() % 256;
            //ispis generisane matrice
            if (i % dimenzija == 0)
                printf("\n");                        
            if (dataBuffer[offset] >= 0 && dataBuffer[offset] < 100)
                printf(" %d ", dataBuffer[offset]);
            else
                printf("%d ", dataBuffer[offset]);                
            ++offset;
        }
        printf("\n");
        int iResult;
        iResult = send(connectSocket, dataBuffer, (int)offset, 0);        
        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }          
    }
    
    iResult = shutdown(connectSocket, SD_BOTH); // Shutdown the connection since we're done    
    if (iResult == SOCKET_ERROR) // Check if connection is succesfully shut down.
    {
        printf("Shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    printf("\nPress any key to exit: ");
    _getch();
    
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

DWORD WINAPI recvOdgovor(LPVOID lpParam)
{    
    int iResult;
    SOCKET connectSocket;
    memcpy(&connectSocket, lpParam, sizeof(SOCKET));
    char buffer[BUFFER_SIZE_REZULTAT];

    while (true)
    {
        iResult = recv(connectSocket, buffer, BUFFER_SIZE_REZULTAT, 0);        
        if (iResult > 0)	
        {
            if (buffer[sizeof(double)])
            {
                printf("\tresenje zahteva<%d>: <%.0f>\n", *(int*)(buffer + sizeof(double) + sizeof(char)), *(double*)buffer);
            }
            else
            {
                printf("\tresenje zahteva<%d>: <neispravno!>\n", *(int*)(buffer + sizeof(double) + sizeof(char)));
            }                       
        }
        else if (iResult == 0)	// Check if shutdown command is received
        {            
            //printf("Connection with server closed.\n"); 
            closesocket(connectSocket); // Connection was closed successfully 
            WSACleanup();
            return 0;
        }
        else	// There was an error during recv
        {
            printf("klient recv failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }
        Sleep(100);
    }
}