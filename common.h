#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *concat(const char *s1, const char *s2);

unsigned long int getAddrOffset(long int pageSizeInByte);

void printRelatory(char* algorithm, char* fileName, unsigned long int memorySize, unsigned long int frameSize, unsigned long int readCount, unsigned long int writeCount, unsigned long int pageFaultsCount, unsigned long int replacetamentsCount);

#endif