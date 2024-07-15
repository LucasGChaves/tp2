#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "page_table_entry.h"
#include "lru.h"
#include "random.h"
#include "second_chance.h"
#include "fifo.h"

unsigned long int frameSize = 0;
unsigned long int memorySize = 0;
unsigned long int numFrames = 0;
unsigned long int pageTableSize = 0;
unsigned long int globalTimestamp = 0;
unsigned long int numFreeFrames = 0;
unsigned long int readCount = 0;
unsigned long int writeCount = 0;
unsigned long int pageFaultsCount = 0;
unsigned long int replacetamentsCount = 0;

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

void initializePageTable(PageTableEntry *pageTable, unsigned long int *freeFrames)
{
    for (unsigned long int i = 0; i < pageTableSize; i++)
    {
        pageTable[i].frameNumber = -1;
        pageTable[i].valid = 0;
        pageTable[i].lastAccess = 0;
    }
    for (unsigned long int i = 0; i < numFrames; i++)
    {
        freeFrames[i] = i;
    }
}

long int findPage(PageTableEntry *pageTable, PageTableEntry page)
{
    unsigned long int pageNumber = -1;

    for (unsigned long int i = 0; i < pageTableSize; i++)
    {
        if (pageTable[i].valid && pageTable[i].frameNumber == page.frameNumber)
        {
            pageNumber = i;
        }
    }
    return pageNumber;
}

long int getRandomPage(PageTableEntry *pageTable, unsigned long int pageTableSize)
{
    unsigned long int pageNumber = -1;
    do
    {
        pageNumber = getRandomNumber(pageTableSize);
    } while (pageTable[pageNumber].valid != 1);
    return pageNumber;
}

int processReplacement(PageTableEntry *pageTable, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue, unsigned long int pageTableSize)
{
    if (strcmp(algorithm, "lru") == 0)
    {
        return findPageWithLru(pageTable, pageTableSize);
    }
    else if (strcmp(algorithm, "2a") == 0)
    {
        PageTableEntry page = dequeueSecondChanceQueue(secondChanceQueue);
        return findPage(pageTable, page);
    }
    else if (strcmp(algorithm, "fifo") == 0)
    {
        PageTableEntry page = dequeue(fifoQueue);
        return findPage(pageTable, page);
    }
    else if (strcmp(algorithm, "random") == 0)
    {
        return getRandomPage(pageTable, pageTableSize);
    }
    return -1;
}

unsigned long int translateAddress(PageTableEntry *pageTable, long int offset, unsigned long int virtualAddress, unsigned long int *freeFrames, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue, unsigned long int pageTableSize)
{
    unsigned long int pageNumber = virtualAddress >> offset;
    globalTimestamp++;
    // printf("CALLING translateAddress -> address: %x(%ld) / pageNumber: %ld / valid: %d / lastAccess: %d / frameNumber: %d\n", virtualAddress, virtualAddress, pageNumber, pageTable[pageNumber].valid, pageTable[pageNumber].lastAccess, pageTable[pageNumber].frameNumber);
    if (pageTable[pageNumber].valid)
    {
        pageTable[pageNumber].lastAccess = globalTimestamp;
        updateReferenceBit(secondChanceQueue, pageTable[pageNumber]);
        return pageTable[pageNumber].frameNumber * frameSize + offset;
    }
    else
    {
        // Page fault
        PageTableEntry page;
        pageFaultsCount++;
        unsigned long int frameNumber;
        if (numFreeFrames > 0)
        {
            frameNumber = freeFrames[--numFreeFrames];
        }
        else
        {
            // Replacement
            replacetamentsCount++;
            unsigned long int replacedPage = processReplacement(pageTable, algorithm, secondChanceQueue, fifoQueue, pageTableSize);
            if (replacedPage == -1)
            {
                printf("No valid page to replace, memory is full\n");
                exit(1);
            }
            frameNumber = pageTable[replacedPage].frameNumber;
            pageTable[replacedPage].valid = 0;
            pageTable[replacedPage].frameNumber = -1;
        }
        page.frameNumber = frameNumber;
        page.valid = 1;
        enqueue(fifoQueue, page);
        enqueueSecondChanceQueue(secondChanceQueue, page);
        pageTable[pageNumber].frameNumber = frameNumber;
        pageTable[pageNumber].valid = 1;
        pageTable[pageNumber].lastAccess = globalTimestamp;
        return pageTable[pageNumber].frameNumber * frameSize + (virtualAddress & ((1 << offset) - 1));
    }
}

unsigned char readMemory(PageTableEntry *pageTable, long int offset, unsigned long int virtualAddress, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue, unsigned long int pageTableSize)
{
    readCount++;
    unsigned long int physicalAddress = translateAddress(pageTable, offset, virtualAddress, freeFrames, algorithm, secondChanceQueue, fifoQueue, pageTableSize);
    return memory[physicalAddress];
}

void writeMemory(PageTableEntry *pageTable, long int offset, unsigned long int virtualAddress, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue, unsigned long int pageTableSize)
{
    writeCount++;
    unsigned long int physicalAddress = translateAddress(pageTable, offset, virtualAddress, freeFrames, algorithm, secondChanceQueue, fifoQueue, pageTableSize);
    memory[physicalAddress] = 'b';
}

void initializeMemory(unsigned char *memory)
{
    for (unsigned long int i = 0; i < memorySize; i++)
    {
        memory[i] = 'a';
    }
}

void processLog(PageTableEntry *pageTable, unsigned long int offset, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, char *filename)
{
    char *filePath = concat("files/", filename);

    FILE *file = fopen(filePath, "r");
    if (!file)
    {
        perror("Error opening file");
        exit(1);
    }
    unsigned long int count = 0;
    char line[256];

    Queue *fifoQueue = createQueue();
    SecondChanceQueue *secondChanceQueue = createSecondChanceQueue();

    while (fgets(line, sizeof(line), file))
    {
        // printf("%ld\n", count++);
        unsigned long int virtualAddress;
        char operation;

        if (sscanf(line, "%lx %c", &virtualAddress, &operation) != 2)
        {
            fprintf(stderr, "Invalid line format: %s", line);
            exit(1);
        }
        // printf("virtualAddress: %lu\n", virtualAddress);
        if (operation == 'R')
        {
            unsigned char value = readMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm, secondChanceQueue, fifoQueue, pageTableSize);
        }
        else
        {
            writeMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm, secondChanceQueue, fifoQueue, pageTableSize);
        }
    }
    fclose(file);
}

void printRelatory(char* algorithm, char* fileName) {
    printf("Executing file %s...\n", fileName);
    printf("Memory size (in bytes): %lu\n", memorySize);
    printf("Frame size (in bytes): %lu\n", frameSize);
    printf("Replacement algorithm: %s\n", algorithm);
    printf("Pages read: %lu\n", readCount);
    printf("Pages written: %lu\n", writeCount);
    printf("Page faults: %lu\n", pageFaultsCount);
    printf("Page replacements: %lu\n", replacetamentsCount);
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
    int offsetInBits = getAddrOffset(frameSizeInByte);
    int bitsReservedToPages = addressSizeInBits - offsetInBits;
    pageTableSize = (1 << bitsReservedToPages); // 2^20

    unsigned char* memory = (unsigned char*) malloc(memorySize * sizeof(unsigned char));

    numFrames = memorySize/frameSize;

    unsigned long int *freeFrames = (unsigned long int *)malloc(numFrames * sizeof(unsigned long int));
    numFreeFrames = numFrames;

    initializeMemory(memory);

    PageTableEntry* pageTable = (PageTableEntry*) malloc(pageTableSize * sizeof(PageTableEntry));

    initializePageTable(pageTable, freeFrames);

    processLog(pageTable, offsetInBits, freeFrames, memory, algorithm, filename);

    printRelatory(algorithm, filename);

    free(pageTable);
    free(memory);
    free(freeFrames);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Execution time in seconds: %f\n", time_spent);
}