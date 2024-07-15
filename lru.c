#include "lru.h"

unsigned long int findPageWithLru(PageTableEntry* pageTable, unsigned long int pageTableSize) {
    unsigned long int lruPage = -1;
    unsigned long int minTimestamp = LONG_MAX;

    for (unsigned long int i = 0; i < pageTableSize; i++) {
        if (pageTable[i].valid && pageTable[i].lastAccess < minTimestamp) {
            minTimestamp = pageTable[i].lastAccess;
            lruPage = i;
        }
    }
    return lruPage;
}