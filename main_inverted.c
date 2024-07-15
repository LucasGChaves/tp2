#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "random.h"
#include "second_chance.h"
#include "fifo.h"
#include "inverted_structs.h"
#include "lru_inverted.h"
#include "common.h"

#define PROCESS_ID 1

unsigned long int frameSize = 0;
unsigned long int memorySize = 0;
unsigned long int numFrames = 0;
unsigned long int pageTableSize = 0;
unsigned long int globalTimestamp = 0;
unsigned long int numFreeFrames = 0;
unsigned int readCount = 0;
unsigned int writeCount = 0;
unsigned int pageFaultsCount = 0;
unsigned int replacetamentsCount = 0;

//Hash to generate index in page table
int hashFunction(InvertedPageTable *table, int processId, unsigned long int pageNumber)
{
    return (processId * pageNumber) % pageTableSize;
}

InvertedPageTable *createInvertedPageTable(unsigned long int *freeFrames)
{
    InvertedPageTable *table = (InvertedPageTable *)malloc(sizeof(InvertedPageTable));
    if (table == NULL)
    {
        perror("Erro ao alocar memória para a tabela de páginas invertida");
        exit(1);
    }

    table->entries = (InvertedPageTableEntry **)calloc(numFrames, sizeof(InvertedPageTableEntry *));
    if (table->entries == NULL)
    {
        perror("Erro ao alocar memória para as entradas da tabela de páginas invertida");
        free(table);
        exit(1);
    }

    for (unsigned long int i = 0; i < numFrames; i++)
    {
        freeFrames[i] = i;
    }

    return table;
}

int insertInvertedPageTableEntry(InvertedPageTable *table, int processId, unsigned long int pageNumber, int frameNumber)
{
    int index = hashFunction(table, processId, pageNumber);
    InvertedPageTableEntry *newEntry = (InvertedPageTableEntry *)malloc(sizeof(InvertedPageTableEntry));
    if (newEntry == NULL)
    {
        perror("Erro ao alocar memória para a nova entrada da tabela de páginas invertida");
        return -1;
    }
    newEntry->processId = processId;
    newEntry->pageNumber = pageNumber;
    newEntry->frameNumber = frameNumber;
    newEntry->valid = 1;
    newEntry->next = table->entries[index]; // Insert at beginning of linked list
    newEntry->lastAccess = globalTimestamp;
    table->entries[index] = newEntry;
    return 1;
}

InvertedPageTableEntry *findInvertedPageTableEntry(InvertedPageTable *table, int processId, unsigned long int pageNumber)
{
    int index = hashFunction(table, processId, pageNumber);
    InvertedPageTableEntry *entry = table->entries[index];
    while (entry != NULL)
    {
        if (entry->processId == processId && entry->pageNumber == pageNumber && entry->valid)
        {
            return entry;
        }
        entry = entry->next;
    }
    return NULL; // Entry not found
}

unsigned long int processReplacement(InvertedPageTable *pageTable, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue)
{
    if (strcmp(algorithm, "lru") == 0)
    {
        return findPageWithLru(pageTable, pageTableSize);
    }
    // else if (strcmp(algorithm, "2a") == 0)
    // {
    //     PageTableEntry page = dequeueSecondChanceQueue(secondChanceQueue);
    //     return findPage(pageTable, page);
    // }
    // else if (strcmp(algorithm, "fifo") == 0)
    // {
    //     PageTableEntry page = dequeue(fifoQueue);
    //     return findPage(pageTable, page);
    // }
    // else if (strcmp(algorithm, "random") == 0)
    // {
    //     return getRandomPage(pageTable, pageTableSize);
    // }
    return -1;
}

long int translateAddress(InvertedPageTable *pageTable, unsigned long int virtualAddress, long int offset,
                          unsigned long int *freeFrames, char *algorithm,
                          SecondChanceQueue *secondChanceQueue, Queue *fifoQueue)
{
    globalTimestamp++;
    unsigned long int pageNumber = virtualAddress >> offset;
    InvertedPageTableEntry *entry = findInvertedPageTableEntry(pageTable, PROCESS_ID, pageNumber);

    if (entry != NULL && entry->valid)
    {
        //Hit
        entry->lastAccess = globalTimestamp;
        return entry->frameNumber * frameSize + (virtualAddress & ((1 << offset) - 1)); // Calcula o endereço físico
    }
    else
    {
        
        //Page fault
        pageFaultsCount++;

        unsigned long int frameNumber;
        if (numFreeFrames > 0)
        {
            frameNumber = freeFrames[--numFreeFrames];
        }
        else
        {
            //Replacement
            replacetamentsCount++;
            frameNumber = processReplacement(pageTable, algorithm, secondChanceQueue, fifoQueue);

            if (frameNumber == -1)
            {
                printf("No valid page to replace, memory is full\n");
                exit(1);
            }

            //Invalidate old entry on inverted table page
            InvertedPageTableEntry *oldEntry = NULL;
            for (int i = 0; i < pageTableSize; i++)
            {
                oldEntry = pageTable->entries[i];
                while (oldEntry != NULL)
                {
                    if (oldEntry->frameNumber == frameNumber && oldEntry->valid)
                    {
                        oldEntry->valid = 0;
                        break;
                    }
                    oldEntry = oldEntry->next;
                }
            }
        }

        if (!insertInvertedPageTableEntry(pageTable, PROCESS_ID, pageNumber, frameNumber))
        {
            perror("Erro ao inserir entrada na tabela de páginas invertida");
            exit(1);
        }
        
        // Update FIFO and Second Chance
        PageTableEntry newPage;

        newPage.frameNumber = frameNumber;
        newPage.valid = 1;
        //enqueue(fifoQueue, newPage);
        //enqueueSecondChanceQueue(secondChanceQueue, newPage);

        return frameNumber * frameSize + (virtualAddress & ((1 << offset) - 1)); // Calculate physical address
    }
}

unsigned char readMemory(InvertedPageTable *pageTable, long int offset, unsigned long int virtualAddress, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue)
{
    readCount++;
    unsigned long int physicalAddress = translateAddress(pageTable, virtualAddress, offset, freeFrames, algorithm, secondChanceQueue, fifoQueue);
    return memory[physicalAddress];
}

void writeMemory(InvertedPageTable *pageTable, long int offset, unsigned long int virtualAddress, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue)
{
    writeCount++;
    unsigned long int physicalAddress = translateAddress(pageTable, virtualAddress, offset, freeFrames, algorithm, secondChanceQueue, fifoQueue);
    memory[physicalAddress] = 'b';
}

void initializeMemory(unsigned char *memory)
{
    for (unsigned long int i = 0; i < memorySize; i++)
    {
        memory[i] = 'a';
    }
}

void processLog(InvertedPageTable *pageTable, unsigned long int offset, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, char *filename)
{
    char *filePath = concat("files/", filename);

    FILE *file = fopen(filePath, "r");
    if (!file)
    {
        perror("Error opening file");
        exit(1);
    }
    char line[256];

    Queue *fifoQueue = NULL;/*createQueue();*/
    SecondChanceQueue *secondChanceQueue = NULL;/*createSecondChanceQueue();*/

    while (fgets(line, sizeof(line), file))
    {
        
        unsigned long int virtualAddress;
        char operation;

        if (sscanf(line, "%lx %c", &virtualAddress, &operation) != 2)
        {
            fprintf(stderr, "Invalid line format: %s", line);
            exit(1);
        }
        
        if (operation == 'R')
        {
            readMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm, secondChanceQueue, fifoQueue);
        }
        else
        {
            writeMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm, secondChanceQueue, fifoQueue);
        }
    }
    fclose(file);
}

int main(int argc, char *argv[])
{
    clock_t begin = clock();
    srand(749);

    if (argc != 5)
    {
        printf("Usage: %s <algorithm> <file> <frame_size> <memory_size>\n", argv[0]);
        return 1;
    }

    char *algorithm = argv[1];
    char *filename = argv[2];

    unsigned int frameSizeInKB = atoi(argv[3]);
    unsigned int frameSizeInByte = frameSizeInKB * 1024;
    frameSize = frameSizeInByte;

    unsigned int memorySizeInKB = atoi(argv[4]);
    unsigned int memorySizeInByte = memorySizeInKB * 1024;
    memorySize = memorySizeInByte;

    int offsetInBits = getAddrOffset(frameSizeInByte);

    unsigned char *memory = (unsigned char *)malloc(memorySize * sizeof(unsigned char));

    numFrames = memorySize / frameSize;

    pageTableSize = numFrames;

    unsigned long int *freeFrames = (unsigned long int *)malloc(numFrames * sizeof(unsigned long int));
    numFreeFrames = numFrames;

    initializeMemory(memory);

    InvertedPageTable *pageTable = createInvertedPageTable(freeFrames);

    processLog(pageTable, offsetInBits, freeFrames, memory, algorithm, filename);
    printRelatory(algorithm, filename, memorySize, frameSize, readCount, writeCount, pageFaultsCount, replacetamentsCount);

    //freeing memory
    for (unsigned long int i = 0; i < pageTableSize; i++) {
        free(pageTable->entries[i]);
    }
    free(pageTable);
    free(memory);
    free(freeFrames);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Execution time in seconds: %f\n", time_spent);
}
