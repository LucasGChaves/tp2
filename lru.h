#include "page_table_entry.h"
#include <limits.h>

unsigned long int findPageWithLru(PageTableEntry* pageTable, unsigned long int pageTableSize);