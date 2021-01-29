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
//#pragma comment(lib, "WS2_32")

#pragma pack(1)

#define SERVER_PORT 27016
#define BUFFER_SIZE 256
#define BUFFER_SIZE_REZULTAT 1024

#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);} 

typedef struct prihvacenaKonekcija {
    SOCKET soket;
    HANDLE handle;
    DWORD nitId;
    struct prihvacenaKonekcija* next;
} prihvacena_konekcija;

typedef struct neobradjenZahtev {
    SOCKET soket;
    int brZahteva;
    char dimenzija;
    char* matrix;
    struct neobradjenZahtev* next;
} neobradjen_zahtev;

typedef struct strukturaZaNit {
    SOCKET soket;
    neobradjen_zahtev** listaNeobradjenihZahteva;
}struktura_za_nit;

CRITICAL_SECTION pristupiListi;

int inicijalizujServer(SOCKET* soket);
DWORD WINAPI obradiZahtev(LPVOID lpParam);
void klientPoslaoMatricu(SOCKET soket, char* buffer, neobradjen_zahtev** listaNeobradjenihZahteva);
int workerZatrazioMatricu(SOCKET soket, neobradjen_zahtev** listaNeobradjenihZahteva);
int workerObradioZahtev(char* buffer);
void Push(neobradjen_zahtev * *listaNeobradjenihZahteva, SOCKET soket, int brZahteva, char dimenzija, char* matrix);
void obrisiPrihvaceneKonekcije(prihvacena_konekcija** listaPrihvacenihKonekcija);
void removeAt(prihvacena_konekcija** head, int pozicija);
void zatvoriAplikaciju(SOCKET* soket);

int main()
{
    InitializeCriticalSection(&pristupiListi);
          
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)        
        return 1;
            
    SOCKET listenSocket = INVALID_SOCKET;
    int iResult = inicijalizujServer(&listenSocket);    
    if (iResult != 0)
        return iResult;
    
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(struct sockaddr_in);    

    neobradjen_zahtev* listaNeobradjenihZahteva = 0;
    prihvacena_konekcija* listaPrihvacenihKonekcija = 0;

    printf("---Server je podignut.---\n");    

    struktura_za_nit data[BUFFER_SIZE];
    int i = 0;
    while (true)
    {        
        prihvacena_konekcija* klient = (prihvacena_konekcija*)malloc(sizeof(prihvacena_konekcija));        
        klient->soket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (klient->soket != INVALID_SOCKET)
        {
            //struktura_za_nit* data = (struktura_za_nit*)malloc(sizeof(struktura_za_nit));  

            //data->listaNeobradjenihZahteva = &listaNeobradjenihZahteva;
            //data->soket = klient->soket; 
            data[i].listaNeobradjenihZahteva = &listaNeobradjenihZahteva;
            data[i].soket = klient->soket;
                        
            klient->handle = CreateThread(NULL, 0, &obradiZahtev, &data[i], 0, &(klient->nitId));
            klient->next = listaPrihvacenihKonekcija;
            listaPrihvacenihKonekcija = klient;            
            ++i;
            continue;
        }     
        free(klient);
    }         

    obrisiPrihvaceneKonekcije(&listaPrihvacenihKonekcija);
    DeleteCriticalSection(&pristupiListi);
    return 0;
}

void zatvoriAplikaciju(SOCKET* soket)
{
    shutdown(*soket, SD_BOTH);
    closesocket(*soket);
    WSACleanup();
}


int inicijalizujServer(SOCKET* soket)
{
    int iResult;
    *soket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*soket == INVALID_SOCKET)
    {
        WSACleanup();
        return 2;
    }

    sockaddr_in serverAddress; // Initialize serverAddress structure used by bind
    memset((char*)&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);

    iResult = bind(*soket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (iResult == SOCKET_ERROR)
    {
        closesocket(*soket);
        WSACleanup();
        return 3;
    }

    iResult = listen(*soket, SOMAXCONN); // Set listenSocket in listening mode
    if (iResult == SOCKET_ERROR)
    {
        closesocket(*soket);
        WSACleanup();
        return 4;
    }
}
DWORD WINAPI obradiZahtev(LPVOID lpParam)
{
    int iResult;    
    struktura_za_nit* data = (struktura_za_nit*)lpParam;
    char buffer[BUFFER_SIZE_REZULTAT];

    while (true)
    {
        iResult = recv(data->soket, buffer, BUFFER_SIZE_REZULTAT, 0);
        if (iResult > 0)
        {
            if (buffer[0] == 1)                            
                klientPoslaoMatricu(data->soket, buffer + sizeof(char), data->listaNeobradjenihZahteva);
            
            else if (buffer[0] == 2 && buffer[1] == 1)            
                workerZatrazioMatricu(data->soket, data->listaNeobradjenihZahteva);
            
            else if (buffer[0] == 2 && buffer[1] == 2)            
                workerObradioZahtev(buffer + 2 * sizeof(char));            
        }
        else if (iResult == 0)	// Check if shutdown command is received
        {            
            closesocket(data->soket);
            return 0;
        }
        else //error	
        {            
            closesocket(data->soket);            
            return 1;
        }
    }
}

void klientPoslaoMatricu(SOCKET soket, char* buffer, neobradjen_zahtev** listaNeobradjenihZahteva)
{
    USHORT port = htons(SERVER_PORT);        
    Push(listaNeobradjenihZahteva,
        soket,
        *(int*)(buffer),
        *(buffer + sizeof(int)),
        (buffer + sizeof(char) + sizeof(int)));

    _spawnl(P_NOWAIT, "../Debug/Worker.exe", (char*)&port, NULL);
}

int workerZatrazioMatricu(SOCKET soket, neobradjen_zahtev** listaNeobradjenihZahteva)
{
    EnterCriticalSection(&pristupiListi);
    neobradjen_zahtev* podaci = *listaNeobradjenihZahteva;
    *listaNeobradjenihZahteva = (*listaNeobradjenihZahteva)->next;
    LeaveCriticalSection(&pristupiListi);
    char offset = 0;
    char dataBuffer[BUFFER_SIZE];    

    int sokete = podaci->soket;

    dataBuffer[0] = 1; //oznaka da server salje poruku
    offset += sizeof(char);
    memcpy(dataBuffer + offset, &sokete, sizeof(SOCKET));    
    offset += sizeof(SOCKET);
    memcpy(dataBuffer + offset, &(podaci->brZahteva), sizeof(int));    
    offset += sizeof(int);
    double mnozilac = 1;
    memcpy(dataBuffer + offset, &mnozilac, sizeof(double));
    offset += sizeof(double);
    memcpy(dataBuffer + offset, &(podaci->dimenzija), sizeof(char));    
    offset += sizeof(char);
    memcpy(dataBuffer + offset, podaci->matrix, sizeof(char) * (podaci->dimenzija) * (podaci->dimenzija));
    offset += sizeof(char) * (podaci->dimenzija) * (podaci->dimenzija);   

    if (send(soket, dataBuffer, (int)offset, 0) == SOCKET_ERROR)
    {        
        closesocket(soket);        
        return 2;
    }

    free(podaci->matrix);
    free(podaci);
    return 0;
}
int workerObradioZahtev(char* buffer)
{
    SOCKET tempSocket = *(int*)(buffer + sizeof(double) + sizeof(char));    
    char tempBuffer[sizeof(double) + sizeof(char) + sizeof(int)];
    int offset = 0;

    memcpy(tempBuffer, (double*)buffer, sizeof(double));
    offset += sizeof(double);
    tempBuffer[offset] = buffer[sizeof(double)];
    offset += sizeof(char);
    memcpy(tempBuffer + offset, (int*)(buffer + sizeof(double) + sizeof(char) + sizeof(SOCKET)), sizeof(int));
    offset += sizeof(int);

    if (send(tempSocket, tempBuffer, (int)offset, 0) == SOCKET_ERROR)
    {        
        closesocket(tempSocket);
        return 3;
    }

    return 0;
}


void obrisiPrihvaceneKonekcije(prihvacena_konekcija** listaPrihvacenihKonekcija)
{
    while (*listaPrihvacenihKonekcija)
    {
        removeAt(listaPrihvacenihKonekcija, 0);
    }
}

void removeAt(prihvacena_konekcija** head, int pozicija)
{
    prihvacena_konekcija* temp = *head;
    if (temp == NULL) {
        return;
    }

    if (pozicija == 0) {
        *head = (*head)->next;
        shutdown(temp->soket, SD_BOTH);
        closesocket(temp->soket);
        SAFE_DELETE_HANDLE(temp->handle);
        free(temp);
        return;
    }
    int i = 1;

    while (temp->next)
    {
        if (i++ == pozicija)
        {
            prihvacena_konekcija* obrisi = temp->next;
            temp->next = temp->next->next;

            shutdown(obrisi->soket, SD_BOTH);
            closesocket(obrisi->soket);
            SAFE_DELETE_HANDLE(obrisi->handle);
            free(obrisi);
            return;
        }
        temp = temp->next;
    }
}

void Push(neobradjen_zahtev** listaNeobradjenihZahteva, SOCKET soket, int brZahteva, char dimenzija, char* matrix)
{    
    neobradjen_zahtev* temp = (neobradjen_zahtev*)malloc(sizeof(neobradjen_zahtev));
    temp->soket = soket;
    temp->brZahteva = brZahteva;
    temp->dimenzija = dimenzija;
    temp->matrix = (char*)malloc(sizeof(char) * dimenzija * dimenzija);
    memcpy(temp->matrix, matrix, sizeof(char) * dimenzija * dimenzija);

    EnterCriticalSection(&pristupiListi);
    temp->next = *listaNeobradjenihZahteva;
    *listaNeobradjenihZahteva = temp;
    LeaveCriticalSection(&pristupiListi);
}
