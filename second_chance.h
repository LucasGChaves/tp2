#ifndef SECOND_CHANCE_H
#define SECOND_CHANCE_H
#include "page_table_entry.h"

typedef struct SecondChanceQueueNode
{
    PageTableEntry page;
    struct SecondChanceQueueNode *next;
    int referenceBit;
} SecondChanceQueueNode;

typedef struct
{
    SecondChanceQueueNode *front;
    SecondChanceQueueNode *rear;
} SecondChanceQueue;

SecondChanceQueue *createSecondChanceQueue();
int isSecondChanceQueueEmpty(SecondChanceQueue *q);
void enqueueSecondChanceQueue(SecondChanceQueue *q, PageTableEntry page);
PageTableEntry dequeueSecondChanceQueue(SecondChanceQueue *q);
void freeSecondChanceQueue(SecondChanceQueue *q);
void updateReferenceBit(SecondChanceQueue *q, PageTableEntry page);
#endif