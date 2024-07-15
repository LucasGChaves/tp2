#ifndef LRU_TWO_LEVELS_H
#define LRU_TWO_LEVELS_H
#include "inverted_structs.h"

unsigned long int findPageWithLru(InvertedPageTable *table, unsigned long int pageTableSize);

#endif