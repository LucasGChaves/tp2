#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "two_levels_structs.h"
#include "lru_two_levels.h"
#include "common.h"

unsigned long int frameSize = 0;
unsigned long int memorySize = 0;
unsigned long int numFrames = 0;
unsigned long int pageTableSize = 0;
unsigned long int firstLevelTableSize = 0;
unsigned long int secondLevelTableSize = 0;
unsigned long int globalTimestamp = 0;
unsigned long int numFreeFrames = 0;
unsigned long int readCount = 0;
unsigned long int writeCount = 0;
unsigned long int pageFaultsCount = 0;
unsigned long int replacetamentsCount = 0;

void initializePageTable(PageTableEntry* pageTable, unsigned long int* freeFrames) {
    for (unsigned long int i = 0; i < pageTableSize; i++) {
        pageTable[i].frameNumber = -1;
        pageTable[i].valid = 0;
        pageTable[i].lastAccess = 0;
    }
    for (unsigned long int i = 0; i < numFrames; i++) {
        freeFrames[i] = i;
    }
}

void initializeFirstLevelPageTable(FirstLevelPageTable* firstLevelPageTable, unsigned long int* freeFrames) {
    firstLevelPageTable->entries = malloc(firstLevelTableSize * sizeof(SecondLevelPageTable*));

    for (unsigned long int i = 0; i < firstLevelTableSize; i++) {
        firstLevelPageTable->entries[i] = malloc(sizeof(SecondLevelPageTable));
        firstLevelPageTable->entries[i]->entries = malloc(secondLevelTableSize * sizeof(PageTableEntry));

        for (unsigned long int j = 0; j < secondLevelTableSize; j++) {
            firstLevelPageTable->entries[i]->entries[j].frameNumber = -1;
            firstLevelPageTable->entries[i]->entries[j].valid = 0;
            firstLevelPageTable->entries[i]->entries[j].lastAccess = 0;
        }
    }

    for (unsigned long int i = 0; i < numFrames; i++) {
        freeFrames[i] = i;
    }
}

void processReplacement(FirstLevelPageTable* firstLevelPageTable, char* algorithm, long int* result) {
    if(strcmp(algorithm, "lru") == 0) {
        findPageWithLru(firstLevelPageTable, firstLevelTableSize, secondLevelTableSize, result);
    }
}

unsigned long int translateAddress(FirstLevelPageTable* firstLevelPageTable, unsigned long int levelOffset, unsigned long int offset, unsigned long int virtualAddress, unsigned long int* freeFrames, char* algorithm) {
    unsigned long int firstLevelIndex = (virtualAddress >> (offset + levelOffset));
    unsigned long int secondLevelIndex = (virtualAddress >> offset) & ((1 << levelOffset) - 1);
    unsigned long int pageOffset = virtualAddress & ((1 << offset) - 1);

    globalTimestamp++;

    if (firstLevelPageTable->entries[firstLevelIndex]->entries[secondLevelIndex].valid) {
        //Hit

        firstLevelPageTable->entries[firstLevelIndex]->entries[secondLevelIndex].lastAccess = globalTimestamp;
        return firstLevelPageTable->entries[firstLevelIndex]->entries[secondLevelIndex].frameNumber * frameSize + pageOffset;
    } else {
        // Page fault

        pageFaultsCount++;
        unsigned long int frameNumber;
        if (numFreeFrames > 0) {
            frameNumber = freeFrames[--numFreeFrames];
        } else {
            //Replacement

            replacetamentsCount++;
            long int *result = (long int*) malloc(sizeof(long int) * 2); 
            result[0] = -1;
            result[1] = -1;
            processReplacement(firstLevelPageTable, algorithm, result);

            if (result[0] == -1 || result[1] == -1) {
                printf("No valid page to replace, memory is full\n");
                exit(1);
            }

            unsigned long int replacedFirstLevelIndex = result[0];
            unsigned long int replacedSecondLevelIndex = result[1];

            frameNumber = firstLevelPageTable->entries[replacedFirstLevelIndex]->entries[replacedSecondLevelIndex].frameNumber;
            firstLevelPageTable->entries[replacedFirstLevelIndex]->entries[replacedSecondLevelIndex].valid = 0;
            firstLevelPageTable->entries[replacedFirstLevelIndex]->entries[replacedSecondLevelIndex].frameNumber = -1;
        }

        firstLevelPageTable->entries[firstLevelIndex]->entries[secondLevelIndex].frameNumber = frameNumber;
        firstLevelPageTable->entries[firstLevelIndex]->entries[secondLevelIndex].valid = 1;
        firstLevelPageTable->entries[firstLevelIndex]->entries[secondLevelIndex].lastAccess = globalTimestamp;
        return firstLevelPageTable->entries[firstLevelIndex]->entries[secondLevelIndex].frameNumber * frameSize + pageOffset;
    }
}

unsigned char readMemory(FirstLevelPageTable* firstLevelPageTable, unsigned long int levelOffset, unsigned long int offset, unsigned long int virtualAddress, unsigned long int* freeFrames, unsigned char* memory, char* algorithm) {
    readCount++;
    unsigned long int physicalAddress = translateAddress(firstLevelPageTable, levelOffset, offset, virtualAddress, freeFrames, algorithm);
    return memory[physicalAddress];
}

void writeMemory(FirstLevelPageTable* firstLevelPageTable, unsigned long int levelOffset, unsigned long int offset,  unsigned long int virtualAddress, unsigned long int* freeFrames, unsigned char* memory, char* algorithm) {
    writeCount++;
    unsigned long int physicalAddress = translateAddress(firstLevelPageTable, levelOffset, offset, virtualAddress, freeFrames, algorithm);
    memory[physicalAddress] = 'b';
}

void initializeMemory(unsigned char* memory) {
    for(unsigned long int i=0; i<memorySize; i++) {
        memory[i] = 'a';
    }
}

void processLog(FirstLevelPageTable* firstLevelPageTable, unsigned long int levelOffset, unsigned long int offset, unsigned long int* freeFrames, unsigned char* memory, char* algorithm, char *filename) {
    char* filePath = concat("files/", filename);

    FILE *file = fopen(filePath, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        unsigned long int virtualAddress;
        char operation;

        if (sscanf(line, "%lx %c", &virtualAddress, &operation) != 2) {
            fprintf(stderr, "Invalid line format: %s", line);
            exit(1);
        }

        if (operation == 'R') {
            readMemory(firstLevelPageTable, levelOffset, offset, virtualAddress, freeFrames, memory, algorithm);
        } else {
            writeMemory(firstLevelPageTable, levelOffset, offset, virtualAddress, freeFrames, memory, algorithm);
        }
    }
    fclose(file);
}

int main(int argc, char *argv[])
{
    clock_t begin = clock();
    if (argc != 5)
    {
        printf("Usage: %s <algorithm> <file> <frame_size> <memory_size>\n", argv[0]);
        return 1;
    }

    char *algorithm = argv[1];
    char *filename = argv[2];

    int addressSizeInBits = 32;

    unsigned int frameSizeInKB = atoi(argv[3]);
    unsigned int frameSizeInByte = frameSizeInKB * 1024;
    frameSize = frameSizeInByte;

    unsigned int memorySizeInKB = atoi(argv[4]);
    unsigned int memorySizeInByte = memorySizeInKB * 1024;
    memorySize = memorySizeInByte;

    unsigned long int offset = getAddrOffset(frameSizeInByte);
    int bitsReservedToPages = addressSizeInBits - offset;

    pageTableSize = (1 << bitsReservedToPages); // 2^20

    unsigned long int levelOffset = bitsReservedToPages / 2; //10 bits for each level

    firstLevelTableSize = 1 << levelOffset;
    secondLevelTableSize = 1 << levelOffset;

    unsigned char* memory = (unsigned char*) malloc(memorySize * sizeof(unsigned char));

    numFrames = memorySize/frameSize;

    unsigned long int* freeFrames = (unsigned long int*) malloc(numFrames * sizeof(unsigned long int));
    numFreeFrames = numFrames;

    initializeMemory(memory);

    FirstLevelPageTable* firstLevelPageTable = (FirstLevelPageTable*) malloc(sizeof(FirstLevelPageTable));
    initializeFirstLevelPageTable(firstLevelPageTable, freeFrames);

    processLog(firstLevelPageTable, levelOffset, offset, freeFrames, memory, algorithm, filename);

    printRelatory(algorithm, filename, memorySize, frameSize, readCount, writeCount, pageFaultsCount, replacetamentsCount);

    //freeing memory
    for (unsigned long int i = 0; i < firstLevelTableSize; i++) {
        free(firstLevelPageTable->entries[i]->entries);
        free(firstLevelPageTable->entries[i]);
    }
    free(firstLevelPageTable);
    free(memory);
    free(freeFrames);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Execution time in seconds: %f\n", time_spent);
}