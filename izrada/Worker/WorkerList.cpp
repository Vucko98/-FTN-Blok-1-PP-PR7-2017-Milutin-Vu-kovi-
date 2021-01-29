#include "WorkerList.h"

//povezi se na onog ko te je napravio
bool connectToBoss(SOCKET* bossSocket, char* port)
{
    *bossSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*bossSocket == INVALID_SOCKET)    
        return true;   

    sockaddr_in bossAddress;
    memset(&bossAddress, 0, sizeof(sockaddr_in));
    bossAddress.sin_family = AF_INET; // IPv4 protocol
    bossAddress.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    memcpy(&bossAddress.sin_port, port, sizeof(USHORT));

    if (connect(*bossSocket, (SOCKADDR*)&bossAddress, sizeof(bossAddress)) == SOCKET_ERROR)
        return true;    

    return false;
}

//ovim se javlja bossu da sam spreman da obradim matricu koju posalje
bool prvaPoruka(SOCKET* soket)
{
    char buffer[2];
    buffer[0] = 2; //worker
    buffer[1] = 1; //prva poruka
    if (send(*soket, buffer, 2, 0) == SOCKET_ERROR)    
        return true;
    
    return false;
}

//workeri pozvani od vrhovnog workera ne pamti podatke o klientu
//offset se postavlja da bi se preskocili podaci o klientu da bi u nastavku funkcije radile uniformno za poruku od servera i workera
//
bool proveriPorekloPoruke(SOCKET* soket, int* offset, char* matrix)
{
    int iResult;
    iResult = recv(*soket, matrix, BUFFER_SIZE, 0);
    if (iResult > 0)
    {
        if (matrix[0] == 1) //serverov potpis
        {
            *offset = sizeof(char) + sizeof(SOCKET) + sizeof(int);//u ovaj if se ulazi samo u slucaju vrhovnog workera(njemu se javio server)
            return false;
        }            
        else if (matrix[0] == 2) //potpis drugog workera
        {
            *offset = sizeof(char);
            return false;
        }
        //ako poruka nije porpisana od servera ili workera, ona je kompromitovana
    }    
    //desila se greska ili prethodni komentar
    return true;
}

//uniformnost na osnovu offset-a
void parametriNaOsnovuPrvePoruke(double* mnozilac, char* dimenzija, char* matrix, int offset)
{
    int tempOffset = offset;
    memcpy(mnozilac, matrix + tempOffset, sizeof(double));
    tempOffset += sizeof(double);

    *dimenzija = matrix[tempOffset];
    tempOffset += sizeof(char);

    memcpy(matrix + offset, matrix + tempOffset, (*dimenzija) * (*dimenzija));
}

double izradunajDeterminantu(char* matrica)
{
    //A11A22A33 + A13A21A32 + A12A23A31- A13A22A31 - A11A23A32 - A12A21A33
    return matrica[0] * matrica[4] * matrica[8]
        + matrica[2] * matrica[3] * matrica[7]
        + matrica[1] * matrica[5] * matrica[6]
        - matrica[2] * matrica[4] * matrica[6]
        - matrica[0] * matrica[5] * matrica[7]
        - matrica[1] * matrica[3] * matrica[8];
}

//na kraju obrade posalji bossu resenje
bool drugaPoruka(SOCKET* soket, double determinanta, char* matrix, bool ispravno)
{   
    int offset = 2 * sizeof(char);
    char buffer[2 * sizeof(char) + sizeof(double) + sizeof(char) + sizeof(SOCKET) + sizeof(int)];
    buffer[0] = 2; //worker
    buffer[1] = 2; //druga poruka    
    memcpy(buffer + offset, &determinanta, sizeof(double));
    offset += sizeof(double);        
    
    if (matrix[0] == 1)//wrhovni vorker ejdini pamti informacije o lkientu. Ovde ih sad koristi da bi se znalo kome pripada resenje
    {
        buffer[offset] = ispravno;
        offset += sizeof(char);             
        memcpy(buffer + offset, matrix + sizeof(char), sizeof(SOCKET));
        offset += sizeof(SOCKET);
        memcpy(buffer + offset, matrix + sizeof(char) + sizeof(SOCKET), sizeof(int));
        offset += sizeof(int);
    }
    
    if (send(*soket, buffer, offset, 0) == SOCKET_ERROR)
        return true;
    
    return false;
}