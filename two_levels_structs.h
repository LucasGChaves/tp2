#ifndef TWO_LEVELS_STRUCTS_H
#define TWO_LEVELS_STRUCTS_H

typedef struct {
    unsigned long int frameNumber;
    int valid;
    unsigned long int lastAccess;
} PageTableEntry;

typedef struct {
    PageTableEntry* entries;
} SecondLevelPageTable;

typedef struct {
    SecondLevelPageTable** entries;
} FirstLevelPageTable;

#endif