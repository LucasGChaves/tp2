#include "random.h"
#include <stdlib.h>

unsigned long int getRandomNumber(int n)
{
    unsigned long int random_value =
        ((unsigned long int)rand() << 24) |
        ((unsigned long int)rand() << 16) |
        ((unsigned long int)rand() << 8) |
        (unsigned long int)rand();

    return random_value % n;
}
