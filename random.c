#include "random.h"
#include <stdlib.h>

int getRandomNumber(int n)
{
    return rand() % n;
}