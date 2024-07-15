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

// Função de hash para gerar o índice na tabela invertida
int hashFunction(InvertedPageTable *table, int processId, unsigned long int pageNumber)
{
    return (processId * pageNumber) % pageTableSize;
}

// Função para criar uma nova tabela de páginas invertida
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

// Função para inserir uma nova entrada na tabela invertida
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
    newEntry->next = table->entries[index]; // Insere no início da lista encadeada
    newEntry->lastAccess = globalTimestamp;
    table->entries[index] = newEntry;
    return 1;
}

// Função para buscar uma entrada na tabela invertida
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
    return NULL; // Entrada não encontrada
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

// Função para traduzir um endereço virtual para um endereço físico
long int translateAddress(InvertedPageTable *pageTable, unsigned long int virtualAddress, long int offset,
                          unsigned long int *freeFrames, char *algorithm,
                          SecondChanceQueue *secondChanceQueue, Queue *fifoQueue)
{
    globalTimestamp++;
    unsigned long int pageNumber = virtualAddress >> offset;
    InvertedPageTableEntry *entry = findInvertedPageTableEntry(pageTable, PROCESS_ID, pageNumber);

    if (entry != NULL && entry->valid)
    {
        // Página encontrada na memória (Page Hit)
        entry->lastAccess = globalTimestamp;
        return entry->frameNumber * frameSize + (virtualAddress & ((1 << offset) - 1)); // Calcula o endereço físico
    }
    else
    {
        
        // Página não encontrada na memória (Page Fault)
        pageFaultsCount++;

        unsigned long int frameNumber;
        if (numFreeFrames > 0)
        {
            // Há quadros livres disponíveis
            frameNumber = freeFrames[--numFreeFrames];
            //printf("frameNumber: %lu\n", frameNumber);
        }
        else
        {
            // Todos os quadros estão ocupados, precisa substituir uma página
            replacetamentsCount++;
            frameNumber = processReplacement(pageTable, algorithm, secondChanceQueue, fifoQueue);

            if (frameNumber == -1)
            {
                printf("No valid page to replace, memory is full\n");
                exit(1);
            }

            // Invalida a entrada antiga na tabela de páginas invertida
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

        // Insere a nova entrada na tabela de páginas invertida
        if (!insertInvertedPageTableEntry(pageTable, PROCESS_ID, pageNumber, frameNumber))
        {
            perror("Erro ao inserir entrada na tabela de páginas invertida");
            exit(1);
        }
        
        // Atualiza FIFO e Second Chance
        PageTableEntry newPage;

        newPage.frameNumber = frameNumber;
        newPage.valid = 1;
        //enqueue(fifoQueue, newPage);
        //enqueueSecondChanceQueue(secondChanceQueue, newPage);

        //printf("frameNumber: %lu / frameSize: %lu / virtualAddress: %lu / offset: %lu\n", frameNumber, frameSize, virtualAddress, offset);
        return frameNumber * frameSize + (virtualAddress & ((1 << offset) - 1)); // Calcula o endereço físico
    }
}

unsigned char readMemory(InvertedPageTable *pageTable, long int offset, unsigned long int virtualAddress, unsigned long int *freeFrames, unsigned char *memory, char *algorithm, SecondChanceQueue *secondChanceQueue, Queue *fifoQueue)
{
    readCount++;
    unsigned long int physicalAddress = translateAddress(pageTable, virtualAddress, offset, freeFrames, algorithm, secondChanceQueue, fifoQueue);
    //"physical address: %lu\n", physicalAddress);
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
    unsigned long int count = 0;
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
            unsigned char value = readMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm, secondChanceQueue, fifoQueue);
        }
        else
        {
            writeMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm, secondChanceQueue, fifoQueue);
        }
        //printf("%ld\n", count++);
    }
    //printf("test\n");
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

    int addressSizeInBits = 32;

    unsigned int frameSizeInKB = atoi(argv[3]);
    unsigned int frameSizeInByte = frameSizeInKB * 1024;
    frameSize = frameSizeInByte;

    unsigned int memorySizeInKB = atoi(argv[4]);
    unsigned int memorySizeInByte = memorySizeInKB * 1024;
    memorySize = memorySizeInByte;

    int offsetInBits = getAddrOffset(frameSizeInByte);
    int bitsReservedToPages = addressSizeInBits - offsetInBits;

    unsigned char *memory = (unsigned char *)malloc(memorySize * sizeof(unsigned char));

    numFrames = memorySize / frameSize;

    pageTableSize = numFrames;

    unsigned long int *freeFrames = (unsigned long int *)malloc(numFrames * sizeof(unsigned long int));
    numFreeFrames = numFrames;

    initializeMemory(memory);

    InvertedPageTable *pageTable = createInvertedPageTable(freeFrames);

    processLog(pageTable, offsetInBits, freeFrames, memory, algorithm, filename);
    printRelatory(algorithm, filename, memorySize, frameSize, readCount, writeCount, pageFaultsCount, replacetamentsCount);

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
