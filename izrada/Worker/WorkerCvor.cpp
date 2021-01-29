#include "WorkerCvor.h"

//inicijalizuj svoj soket i stavi ga u listen
bool becomeBoss(SOCKET* mojSocket, sockaddr_in* workerAddress)
{
    *mojSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*mojSocket == INVALID_SOCKET)
        return true;

    memset((char*)workerAddress, 0, sizeof(sockaddr_in));
    workerAddress->sin_family = AF_INET;
    workerAddress->sin_addr.s_addr = inet_addr(IP_ADDRESS);
    workerAddress->sin_port = 0; //(auto assign port)

    if (bind(*mojSocket, (struct sockaddr*)workerAddress, sizeof(sockaddr_in)) == SOCKET_ERROR)
        return true;

    //// All connections are by default accepted by protocol stek if socket is in listening mode.
    //// With SO_CONDITIONAL_ACCEPT parameter set to true, connections will not be accepted by default
    bool bOptVal = true;
    int bOptLen = sizeof(bool);
    if (setsockopt(*mojSocket, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (char*)&bOptVal, bOptLen) == SOCKET_ERROR)
        return true;

    unsigned long  mode = 1;
    if (ioctlsocket(*mojSocket, FIONBIO, &mode) != 0)
        return true;

    if (listen(*mojSocket, SOMAXCONN) == SOCKET_ERROR)
        return true;

    return false;
}

//posalji tempMatrix(podmatricu) workeru koji ju je zatrazio
bool odgovorNaPrvuPoruku(SOCKET * soket, char* tempMatrix, int offset)
{
    if (send(*soket, tempMatrix, offset, 0) == SOCKET_ERROR)
        return true;

    return false;
}

//matrix je matrica
//tempMatrix iteracija podmatrice od matrixa
//mnozilac je mnozilac matrice od koje se sad pravi podmatrica
//zbog toga se mnozi sa "nultom" kolonom
int podmatrica(char iteracija, double mnozilac, char dimenzija, char* matrix, char* tempMatrix)
{
    tempMatrix[0] = 2; //potpis Workera
    int offset = sizeof(char);
    mnozilac *= matrix[iteracija];
    if (iteracija % 2 == 1)//iz formule za dobijanje determinante
        mnozilac *= -1;
    memcpy(tempMatrix + offset, &mnozilac, sizeof(double));
    offset += sizeof(double);
    --dimenzija;
    memcpy(tempMatrix + offset, &dimenzija, sizeof(char));
    offset += sizeof(char);
    ++dimenzija;
    for (char kolona = 1; kolona < dimenzija; kolona++)
    {
        for (char vrsta = 0; vrsta < dimenzija; vrsta++)
        {
            if (vrsta == iteracija)//iz formule za dobijanje determinante
                continue;
            tempMatrix[offset++] = matrix[kolona * dimenzija + vrsta];
        }
    }
    return offset;
}