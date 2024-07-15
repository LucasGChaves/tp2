#include "lru_two_levels.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

void findPageWithLru(FirstLevelPageTable* firstLevelPageTable, unsigned long int numFirstLevelEntries, unsigned long int numSecondLevelEntries, long int* result) {
    unsigned long int minTimestamp = ULONG_MAX;
    unsigned long int firstLevelIndex = 0, secondLevelIndex = 0;
    for (unsigned long int i = 0; i < numFirstLevelEntries; i++) {
        SecondLevelPageTable* secondLevelPageTable = firstLevelPageTable->entries[i];
        if (secondLevelPageTable != NULL) {
            for (unsigned long int j = 0; j < numSecondLevelEntries; j++) {
                if (secondLevelPageTable->entries[j].valid && secondLevelPageTable->entries[j].lastAccess < minTimestamp) {
                    minTimestamp = secondLevelPageTable->entries[j].lastAccess;
                    firstLevelIndex = i;
                    secondLevelIndex = j;
                }
            }
        }
    }
    result[0] = firstLevelIndex;
    result[1] = secondLevelIndex;
}