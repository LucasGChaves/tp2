#ifndef INVERTED_STRUCTS_H
#define INVERTED_STRUCTS_H


typedef struct InvertedPageTableEntry
{
    int processId;
    unsigned long int pageNumber;
    unsigned long int frameNumber;
    int valid;
    unsigned long int lastAccess;
    struct InvertedPageTableEntry *next; 
} InvertedPageTableEntry;


typedef struct InvertedPageTable
{
    int tableSize;
    InvertedPageTableEntry **entries;
} InvertedPageTable;

#endif
