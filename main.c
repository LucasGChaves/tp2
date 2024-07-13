#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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

typedef struct {
    unsigned long int frameNumber;
    int valid;
    unsigned long int lastAccess;
} PageTableEntry;

char *concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

long int getAddrOffset(long int pageSizeInByte)
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

long int findPageWithLru(PageTableEntry* pageTable) {
    unsigned long int lruPage = -1;
    unsigned long int minTimestamp = INT_MAX;

    for (unsigned long int i = 0; i < pageTableSize; i++) {
        if (pageTable[i].valid && pageTable[i].lastAccess < minTimestamp) {
            minTimestamp = pageTable[i].lastAccess;
            lruPage = i;
        }
    }
    return lruPage;
}

int processReplacement(PageTableEntry* pageTable, char* algorithm) {
    if(strcmp(algorithm, "lru") == 0) {
        return findPageWithLru(pageTable);
    }
}


long int translateAddress(PageTableEntry* pageTable, long int offset, unsigned long int virtualAddress, unsigned long int* freeFrames, char* algorithm) {
    unsigned long int pageNumber = virtualAddress >> offset;
    globalTimestamp++;
    //printf("CALLING translateAddress -> address: %x(%ld) / pageNumber: %ld / valid: %d / lastAccess: %d / frameNumber: %d\n", virtualAddress, virtualAddress, pageNumber, pageTable[pageNumber].valid, pageTable[pageNumber].lastAccess, pageTable[pageNumber].frameNumber);
    if (pageTable[pageNumber].valid) {
        pageTable[pageNumber].lastAccess = globalTimestamp;
        return pageTable[pageNumber].frameNumber * frameSize + offset;
    } else {
        //Page fault
        pageFaultsCount++;
        unsigned long int frameNumber;
        if(numFreeFrames > 0) {
            frameNumber = freeFrames[--numFreeFrames];
        }
        else{
            //Replacement
            replacetamentsCount++;
            unsigned long int replacedPage = processReplacement(pageTable, algorithm);
            if(replacedPage == -1) {
                printf("No valid page to replace, memory is full\n");
                exit(1);
            }
            frameNumber = pageTable[replacedPage].frameNumber;
            pageTable[replacedPage].valid = 0;
            pageTable[replacedPage].frameNumber = -1;
        }

        pageTable[pageNumber].frameNumber = frameNumber;
        pageTable[pageNumber].valid = 1;
        pageTable[pageNumber].lastAccess = globalTimestamp;
        //printf("pageNumber: %ld / valid: %d / freeNumber: %d\n",  pageNumber, pageTable[pageNumber].valid, pageTable[pageNumber].frameNumber);
        return pageTable[pageNumber].frameNumber * frameSize + offset;
    }
}

unsigned char readMemory(PageTableEntry* pageTable, long int offset, unsigned long int virtualAddress, unsigned long int* freeFrames, unsigned char* memory, char* algorithm) {
    readCount++;
    unsigned long int physicalAddress = translateAddress(pageTable, offset, virtualAddress, freeFrames, algorithm);
    return memory[physicalAddress];
}

void writeMemory(PageTableEntry* pageTable, long int offset,  unsigned long int virtualAddress, unsigned long int* freeFrames, unsigned char* memory, char* algorithm) {
    writeCount++;
    unsigned long int physicalAddress = translateAddress(pageTable, offset, virtualAddress, freeFrames, algorithm);
    memory[physicalAddress] = 0xAB;
}

void initializeMemory(unsigned char* memory) {
    for(unsigned long int i=0; i<memorySize; i++) {
        memory[i] = 0xAC;
    }
}

void processLog(PageTableEntry* pageTable, unsigned long int offset, unsigned long int* freeFrames, unsigned char* memory, char* algorithm, char *filename) {
    char* filePath = concat("files/", filename);

    FILE *file = fopen(filePath, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }
    unsigned long int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
       //printf("%d\n", count++);
        unsigned long int virtualAddress;
        char operation;

        if (sscanf(line, "%lx %c", &virtualAddress, &operation) != 2) {
            fprintf(stderr, "Invalid line format: %s", line);
            exit(1);
        }
        //printf("virtualAddress: %lu\n", virtualAddress);
        if (operation == 'R') {
            unsigned char value = readMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm);
        } else {
            writeMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm);
        }
    }
    printf("test\n");
    fclose(file);
}

void printRelatory(char* algorithm, char* fileName) {
    printf("Executando o arquivo %s...\n", fileName);
    printf("Tamanho da memoria: %ld\n", memorySize);
    printf("Tamanho dos frames: %ld\n", frameSize);
    printf("Tecnica de reposicao: %s\n", algorithm);
    printf("Paginas lidas: %ld\n", readCount);
    printf("Pagnas escritas: %ld\n", writeCount);
    printf("Page faults: %ld\n", pageFaultsCount);
    printf("Substituicoes de pagina: %ld\n", replacetamentsCount);
}

int main(int argc, char *argv[])
{
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

    //printf("%ld", memorySize);
    unsigned char* memory = (unsigned char*) malloc(memorySize * sizeof(unsigned char));
    //printf("\n0\n");
    numFrames = memorySize/frameSize;

    unsigned long int* freeFrames = (unsigned long int*) malloc(numFrames * sizeof(unsigned long int));
    numFreeFrames = numFrames;
    //printf("\n1\n");
    initializeMemory(memory);
    //printf("\n2\n");
    //printf("%d\n", pageTableSize);
    PageTableEntry* pageTable = (PageTableEntry*) malloc(pageTableSize * sizeof(PageTableEntry));
    //printf("\n3\n");
    initializePageTable(pageTable, freeFrames);
    //printf("\n4 - %s\n", filename);
    processLog(pageTable, offsetInBits, freeFrames, memory, algorithm, filename);
    //printf("\n5\n");
    printRelatory(algorithm, filename);
    //printf("\n6\n");
    free(pageTable);
    free(memory);
    free(freeFrames);
}