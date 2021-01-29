#include "Common.h"

bool connectToBoss(SOCKET* bossSocket, char* port);
bool prvaPoruka(SOCKET* port);
bool proveriPorekloPoruke(SOCKET* soket, int* offset, char* matrix);
void parametriNaOsnovuPrvePoruke(double* mnozilac, char* dimenzija, char* matrix, int offset);
double izradunajDeterminantu(char*);
bool drugaPoruka(SOCKET* soket, double determinanta, char* matrix, bool ispravno);