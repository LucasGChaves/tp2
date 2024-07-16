#include <stdio.h>
#include <stdlib.h>
#include "random.h"
#include "second_chance.h"
#include "fifo.h"

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

typedef struct InvertedPageTableEntry
{
    int processId;
    unsigned long int pageNumber;
    int frameNumber;
    int valid;
    struct InvertedPageTableEntry *next;
} InvertedPageTableEntry;

typedef struct InvertedPageTable
{
    int tableSize;
    InvertedPageTableEntry **entries;
} InvertedPageTable;

int hashFunction(InvertedPageTable *table, int processId, unsigned long int pageNumber)
{
    return (processId * pageNumber) % table->tableSize;
}

InvertedPageTable *createInvertedPageTable(int numFrames)
{
    InvertedPageTable *table = (InvertedPageTable *)malloc(sizeof(InvertedPageTable));
    if (table == NULL)
    {
        perror("Erro ao alocar memória para a tabela de páginas invertida");
        exit(1);
    }
    table->tableSize = numFrames;
    table->entries = (InvertedPageTableEntry **)calloc(numFrames, sizeof(InvertedPageTableEntry *));
    if (table->entries == NULL)
    {
        perror("Erro ao alocar memória para as entradas da tabela de páginas invertida");
        free(table);
        exit(1);
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
    newEntry->next = table->entries[index];
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
    return NULL;
}

int processReplacement(InvertedPageTable *pageTable, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue, unsigned long int pageTableSize)
{
    if (strcmp(algorithm, "lru") == 0)
    {
        return findPageWithLru(pageTable);
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

long int translateAddress(InvertedPageTable *pageTable, unsigned long int virtualAddress, long int offset,
                          unsigned long int *freeFrames, int *numFreeFrames, char *algorithm,
                          SecondChanceQueue *secondChanceQueue, Queue *fifoQueue)
{

    unsigned long int pageNumber = virtualAddress >> offset;
    InvertedPageTableEntry *entry = findInvertedPageTableEntry(pageTable, PROCESS_ID, pageNumber);

    if (entry != NULL && entry->valid)
    {
        // Page Hit
        return entry->frameNumber * frameSize + (virtualAddress & ((1 << offset) - 1));
    }
    else
    {
        // Page Fault
        pageFaultsCount++;

        int frameNumber;
        if (*numFreeFrames > 0)
        {
            frameNumber = freeFrames[--(*numFreeFrames)];
        }
        else
        {
            replacetamentsCount++;
            frameNumber = processReplacement(pageTable, algorithm, secondChanceQueue, fifoQueue, pageTable->tableSize);
            if (frameNumber == -1)
            {
                printf("No valid page to replace, memory is full\n");
                exit(1);
            }

            InvertedPageTableEntry *oldEntry = NULL;
            for (int i = 0; i < pageTable->tableSize; i++)
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

        PageTableEntry newPage;
        newPage.frameNumber = frameNumber;
        newPage.valid = 1;
        enqueue(fifoQueue, newPage);
        enqueueSecondChanceQueue(secondChanceQueue, newPage);

        return frameNumber * frameSize + (virtualAddress & ((1 << offset) - 1));
    }
}

unsigned char readMemory(InvertedPageTable *pageTable, long int offset, unsigned long int virtualAddress, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue, unsigned long int pageTableSize)
{
    readCount++;
    unsigned long int physicalAddress = translateAddress(pageTable, offset, virtualAddress, freeFrames, algorithm, secondChanceQueue, fifoQueue, pageTableSize);
    return memory[physicalAddress];
}

void writeMemory(InvertedPageTable *pageTable, long int offset, unsigned long int virtualAddress, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue, unsigned long int pageTableSize)
{
    writeCount++;
    unsigned long int physicalAddress = translateAddress(pageTable, offset, virtualAddress, freeFrames, algorithm, secondChanceQueue, fifoQueue, pageTableSize);
    memory[physicalAddress] = 0xAB;
}

void initializeMemory(unsigned char *memory)
{
    for (unsigned long int i = 0; i < memorySize; i++)
    {
        memory[i] = 0xAC;
    }
}

void processLog(InvertedPageTable *pageTable, unsigned long int offset, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, char *filename, unsigned long int pageTableSize)
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
        if (operation == 'R')
        {
            unsigned char value = readMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm, secondChanceQueue, fifoQueue, pageTableSize);
        }
        else
        {
            writeMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm, secondChanceQueue, fifoQueue, pageTableSize);
        }
    }
    printf("test\n");
    fclose(file);
}

void printRelatory(char *algorithm, char *fileName)
{
    printf("Executando o arquivo %s...\n", fileName);
    printf("Tamanho da memoria: %ld\n", memorySize);
    printf("Tamanho dos frames: %ld\n", frameSize);
    printf("Tecnica de reposicao: %s\n", algorithm);
    printf("Paginas lidas: %d\n", readCount);
    printf("Pagnas escritas: %d\n", writeCount);
    printf("Page faults: %d\n", pageFaultsCount);
    printf("Substituicoes de pagina: %d\n", replacetamentsCount);
}

int main(int argc, char *argv[])
{

    srand(749);

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
    pageTableSize = (1 << bitsReservedToPages);

    unsigned char *memory = (unsigned char *)malloc(memorySize * sizeof(unsigned char));

    numFrames = memorySize / frameSize;

    unsigned long int *freeFrames = (unsigned long int *)malloc(numFrames * sizeof(unsigned long int));
    numFreeFrames = numFrames;

    initializeMemory(memory);

    InvertedPageTable *pageTable = createInvertedPageTable(numFrames);

    processLog(pageTable, offsetInBits, freeFrames, memory, algorithm, filename, pageTableSize);
    printRelatory(algorithm, filename);

    free(pageTable);
    free(memory);
    free(freeFrames);
}
