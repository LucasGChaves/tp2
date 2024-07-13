#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

unsigned int frameSize = 0;
unsigned int memorySize = 0;
unsigned int numFrames = 0;
unsigned int pageTableSize = 0;
unsigned int globalTimestamp = 0;
unsigned int numFreeFrames = 0;
unsigned int readCount = 0;
unsigned int writeCount = 0;
unsigned int pageFaultsCount = 0;
unsigned int replacetamentsCount = 0;

typedef struct {
    long frameNumber;
    int valid;
    long int lastAccess;
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

void initializePageTable(PageTableEntry* pageTable, int* freeFrames) {
    for (int i = 0; i < pageTableSize; i++) {
        pageTable[i].frameNumber = -1;
        pageTable[i].valid = 0;
        pageTable[i].lastAccess = 0;
    }
    for (int i = 0; i < numFrames; i++) {
        freeFrames[i] = i;
    }
}

long int findPageWithLru(PageTableEntry* pageTable) {
    long int lruPage = -1;
    unsigned int minTimestamp = INT_MAX;

    for (int i = 0; i < pageTableSize; i++) {
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


long int translateAddress(PageTableEntry* pageTable, long int offset, long int virtualAddress, int* freeFrames, char* algorithm) {
    long int pageNumber = virtualAddress >> offset;
    globalTimestamp++;
    //printf("CALLING translateAddress -> address: %x(%ld) / pageNumber: %ld / valid: %d / lastAccess: %d / frameNumber: %d\n", virtualAddress, virtualAddress, pageNumber, pageTable[pageNumber].valid, pageTable[pageNumber].lastAccess, pageTable[pageNumber].frameNumber);
    if (pageTable[pageNumber].valid) {
        pageTable[pageNumber].lastAccess = globalTimestamp;
        return pageTable[pageNumber].frameNumber * frameSize + offset;
    } else {
        //Page fault
        pageFaultsCount++;
        int frameNumber;
        if(numFreeFrames > 0) {
            frameNumber = freeFrames[--numFreeFrames];
        }
        else{
            //Replacement
            replacetamentsCount++;
            int replacedPage = processReplacement(pageTable, algorithm);
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

unsigned char readMemory(PageTableEntry* pageTable, long int offset, long int virtualAddress, int* freeFrames, unsigned char* memory, char* algorithm) {
    readCount++;
    long physicalAddress = translateAddress(pageTable, offset, virtualAddress, freeFrames, algorithm);
    long int page = virtualAddress >> offset;
    return memory[physicalAddress];
}

void writeMemory(PageTableEntry* pageTable, long int offset,  long int virtualAddress, int* freeFrames, unsigned char* memory, char* algorithm) {
    writeCount++;
    int physicalAddress = translateAddress(pageTable, offset, virtualAddress, freeFrames, algorithm);
    memory[physicalAddress] = 0xAB;
}

void initializeMemory(unsigned char* memory) {
    for(long int i=0; i<memorySize; i++) {
        memory[i] = 0xAC;
    }
}

void processLog(PageTableEntry* pageTable, long int offset, int* freeFrames, unsigned char* memory, char* algorithm, char *filename) {
    char* filePath = concat("files/", filename);

    FILE *file = fopen(filePath, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        unsigned int virtualAddress;
        char operation;

        if (sscanf(line, "%x %c", &virtualAddress, &operation) != 2) {
            fprintf(stderr, "Invalid line format: %s", line);
            exit(1);
        }

        if (operation == 'R') {
            unsigned char value = readMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm);
        } else {
            writeMemory(pageTable, offset, virtualAddress, freeFrames, memory, algorithm);
        }
    }

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
    long int frameSizeInByte = frameSizeInKB * 1024;
    frameSize = frameSizeInByte;

    unsigned int memorySizeInKB = atoi(argv[4]);
    long int memorySizeInByte = memorySizeInKB * 1024;
    memorySize = memorySizeInByte;
    
    int offsetInBits = getAddrOffset(frameSizeInByte);
    int bitsReservedToPages = addressSizeInBits - offsetInBits;
    pageTableSize = (1 << bitsReservedToPages); // 2^20

    //printf("%ld", memorySize);
    unsigned char* memory = (unsigned char*) malloc(memorySize * sizeof(unsigned char));
    //printf("\n0\n");
    numFrames = memorySize/frameSize;

    int* freeFrames = (int*) malloc(numFrames * sizeof(int));
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