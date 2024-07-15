#ifndef PAGE_TABLE_ENTRY
#define PAGE_TABLE_ENTRY

typedef struct
{
    unsigned long int frameNumber;
    int valid;
    unsigned long int lastAccess;
} PageTableEntry;

#endif