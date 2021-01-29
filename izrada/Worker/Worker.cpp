#include "WorkerCvor.h"
#include "WorkerList.h"
#include "CloseFunctions.h"

int main(int argc, char* argv[])
{    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 1;
    
    SOCKET bossSocket = INVALID_SOCKET;        

    if (connectToBoss(&bossSocket, argv[0]))
    {
        zatvoriAplikaciju(&bossSocket);
        return 1;
    }
    if (prvaPoruka(&bossSocket))
    {
        zatvoriAplikaciju(&bossSocket);
        return 2;
    }
    
    char matrica[BUFFER_SIZE];
    int offset;

    if (proveriPorekloPoruke(&bossSocket, &offset, matrica))
    {
        zatvoriAplikaciju(&bossSocket);
        return 3;
    }

    char dimenzija;
    double mnozilac;
    parametriNaOsnovuPrvePoruke(&mnozilac, &dimenzija, matrica, offset);
    //dosao sam dovde
    double determinanta = 0;
    bool ispravnaObradaDeterminante = true;
    if (dimenzija == 1)
        determinanta = matrica[9];

    else if (dimenzija == 2)
        determinanta = matrica[9] * matrica[12] - matrica[10] * matrica[11];

    else if (dimenzija == 3)
        determinanta = mnozilac * izradunajDeterminantu(matrica + offset);    

    else if(dimenzija > 3)
    {        
        SOCKET mojSocket = INVALID_SOCKET;                        
        sockaddr_in workerAddress;  
        if (becomeBoss(&mojSocket, &workerAddress))
        {
            closesocket(mojSocket);
            zatvoriAplikaciju(&bossSocket);
            return 12;
        }
        
        USHORT mojPort = 0;
        socklen_t len = sizeof(workerAddress);
        if (getsockname(mojSocket, (struct sockaddr*)&workerAddress, &len) != INVALID_SOCKET)
            mojPort = ntohs(workerAddress.sin_port);
        mojPort = htons(mojPort);

        char i, j;        
        SOCKET* workerSockets = (SOCKET*)malloc(sizeof(SOCKET) * dimenzija); //ne zaboravi. free        
        for (i = 0; i < dimenzija; i++)
            workerSockets[i] = INVALID_SOCKET;

        fd_set readfds; // set of socket descriptors

        timeval timeVal; // timeout for select function
        timeVal.tv_sec = 5;
        timeVal.tv_usec = 1000;

        int iResult;
        char iteracija = 0;
        char brZahteva = 0;        
        char lastIndex = 0;
        char brOdgovora = 0;
        char selectResult;
        sockaddr_in tempWorkerAddr;
        int tempWorkerAddrSize = sizeof(struct sockaddr_in);
        unsigned long  mode = 1;        
        char* tempMatrix = (char*)malloc(sizeof(char) + sizeof(double) + sizeof(char) + sizeof(char) * (dimenzija - 1) * (dimenzija - 1));        
        
        for (i = 0; i < dimenzija; i++)
            _spawnl(P_NOWAIT, "../Debug/Worker.exe", (char*)&mojPort, NULL);       

        while (brOdgovora!=dimenzija)
        {    
            FD_ZERO(&readfds); // initialize socket set
            
            if (brZahteva != dimenzija)
                FD_SET(mojSocket, &readfds);
            
            for (i = 0; i < lastIndex; i++)
                FD_SET(workerSockets[i], &readfds);
            
            selectResult = select(0, &readfds, NULL, NULL, &timeVal);
            if (selectResult == SOCKET_ERROR)
            {   
                zatvoriBossWorkera(&bossSocket, &mojSocket, &workerSockets, dimenzija, &tempMatrix);
                return 5;                
            }
            else if (selectResult == 0) // timeout expired
            {
                zatvoriBossWorkera(&bossSocket, &mojSocket, &workerSockets, dimenzija, &tempMatrix);
                return 6;                
            }
            else if (FD_ISSET(mojSocket, &readfds))
            {
                workerSockets[lastIndex] = accept(mojSocket, (struct sockaddr*)&tempWorkerAddr, &tempWorkerAddrSize);
                if (workerSockets[lastIndex] == INVALID_SOCKET)
                {
                    zatvoriBossWorkera(&bossSocket, &mojSocket, &workerSockets, dimenzija, &tempMatrix);
                    return 7;
                }
                else
                {
                    if (ioctlsocket(workerSockets[lastIndex], FIONBIO, &mode) != 0)
                    {
                        zatvoriBossWorkera(&bossSocket, &mojSocket, &workerSockets, dimenzija, &tempMatrix);
                        return 8;
                    }                                        
                    ++brZahteva;
                    ++lastIndex;
                }
            }
            else
            {
                for (int i = 0; i < lastIndex; i++)
                {
                    if (FD_ISSET(workerSockets[i], &readfds))
                    {
                        iResult = recv(workerSockets[i], tempMatrix, BUFFER_SIZE, 0);
                        if (iResult > 0)
                        {
                            if (tempMatrix[0] == 2 && tempMatrix[1] == 1)
                            {                                                                                                    
                                iResult = podmatrica(iteracija++, mnozilac, dimenzija, matrica + offset, tempMatrix);
                                if (odgovorNaPrvuPoruku(&workerSockets[i], tempMatrix, iResult))
                                {
                                    zatvoriBossWorkera(&bossSocket, &mojSocket, &workerSockets, dimenzija, &tempMatrix);
                                    return 9;
                                }
                            }
                            else if (tempMatrix[0] == 2 && tempMatrix[1] == 2)//sinhronizuj!!!///////////////////////////////////////////////////////////////////////////
                            {
                                determinanta += *(double*)&tempMatrix[2];                                
                            }
                            else
                            {
                                zatvoriBossWorkera(&bossSocket, &mojSocket, &workerSockets, dimenzija, &tempMatrix);
                                return 10;
                            }
                        }
                        else if (iResult == 0) // connection was closed gracefully
                        {                            
                            ++brOdgovora;
                            closesocket(workerSockets[i]);
                            
                            for (j = i; j < lastIndex - 1; j++) // sort array and clean last place                            
                                workerSockets[j] = workerSockets[j + 1];                            
                            workerSockets[--lastIndex] = 0;                            
                        }
                        else
                        {                            
                            zatvoriBossWorkera(&bossSocket, &mojSocket, &workerSockets, dimenzija, &tempMatrix);
                            return 11;
                        }
                    }
                }                
            }                
        }

        free(workerSockets);
        free(tempMatrix);
    }
    else
    {        
        ispravnaObradaDeterminante = false;        
    }
   
    if (drugaPoruka(&bossSocket, determinanta, matrica, ispravnaObradaDeterminante))
    {
        zatvoriAplikaciju(&bossSocket);
        return 4;
    }

    zatvoriAplikaciju(&bossSocket);
    return 0;      
}