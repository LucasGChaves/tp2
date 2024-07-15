#include "lru_inverted.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

unsigned long int findPageWithLru(InvertedPageTable *table, unsigned long int pageTableSize) {

    unsigned long int lruFrame = -1;
    unsigned long int minTimestamp = ULONG_MAX;
    
    for (unsigned long int i = 0; i < pageTableSize; i++) {
        InvertedPageTableEntry *entry = table->entries[i];
        while(entry != NULL) {
            if (entry->valid && entry->lastAccess < minTimestamp) {
                minTimestamp = entry->lastAccess;
                lruFrame = entry->frameNumber;
            }
            entry = entry->next;
        }

    }
    return lruFrame;
}