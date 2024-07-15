#ifndef FIFO
#define FIFO
#include "page_table_entry.h"

typedef struct Node
{
    PageTableEntry page;
    struct Node *next;
} Node;

typedef struct
{
    Node *front;
    Node *rear;
} Queue;

Queue *createQueue();
int isEmpty(Queue *q);
void enqueue(Queue *q, PageTableEntry page);
PageTableEntry dequeue(Queue *q);
void freeQueue(Queue *q);

#endif