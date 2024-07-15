#ifndef LRU_TWO_LEVELS_H
#define LRU_TWO_LEVELS_H
#include "two_levels_structs.h"

void findPageWithLru(FirstLevelPageTable* firstLevelPageTable, unsigned long int numFirstLevelEntries, unsigned long int numSecondLevelEntries, long int* result);

#endif