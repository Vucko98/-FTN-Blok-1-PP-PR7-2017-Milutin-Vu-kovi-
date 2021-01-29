#include "CloseFunctions.h"
#include "WorkerList.h"

void zatvoriBossWorkera(SOCKET* bossSocket, SOCKET* mojSocket, SOCKET** zauzetiSocketi, char dimenzija, char** zauzetBafer)
{
    drugaPoruka(bossSocket, 0, 0, false);
    closesocket(*mojSocket);
    for (int i = 0; i < dimenzija; i++)
    {
        if ((*zauzetiSocketi)[i] == INVALID_SOCKET)
            continue;
        closesocket((*zauzetiSocketi)[i]);
    }
    free(*zauzetiSocketi);
    free(*zauzetBafer);
    zatvoriAplikaciju(bossSocket);
}

void zatvoriAplikaciju(SOCKET* soket)
{
    shutdown(*soket, SD_BOTH);
    closesocket(*soket);
    WSACleanup();
}
