#include "common.h"

char *concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

unsigned long int getAddrOffset(long int pageSizeInByte)
{
    long int tmp = pageSizeInByte;
    long int offset = 0;
    while (tmp > 1)
    {
        tmp = tmp >> 1;
        offset++;
    }
    return offset;
}

void printRelatory(char* algorithm, char* fileName, unsigned long int memorySize, unsigned long int frameSize, unsigned long int readCount, unsigned long int writeCount, unsigned long int pageFaultsCount, unsigned long int replacetamentsCount) {
    printf("Executing file %s...\n", fileName);
    printf("Memory size (in bytes): %lu\n", memorySize);
    printf("Frame size (in bytes): %lu\n", frameSize);
    printf("Replacement algorithm: %s\n", algorithm);
    printf("Pages read: %lu\n", readCount);
    printf("Pages written: %lu\n", writeCount);
    printf("Page faults: %lu\n", pageFaultsCount);
    printf("Page replacements: %lu\n", replacetamentsCount);
}