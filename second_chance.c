#include <stdio.h>
#include <stdlib.h>
#include "second_chance.h"

SecondChanceQueue *createSecondChanceQueue()
{
    SecondChanceQueue *q = (SecondChanceQueue *)malloc(sizeof(SecondChanceQueue));
    if (q == NULL)
    {
        printf("Error allocating memory for the queue!\n");
        exit(1);
    }
    q->front = q->rear = NULL;
    return q;
}

int isSecondChanceQueueEmpty(SecondChanceQueue *q)
{
    return q->front == NULL;
}

//Enqueue page
void enqueueSecondChanceQueue(SecondChanceQueue *q, PageTableEntry page)
{
    SecondChanceQueueNode *newNode = (SecondChanceQueueNode *)malloc(sizeof(SecondChanceQueueNode));
    if (newNode == NULL)
    {
        perror("Erro ao alocar memória para o novo nó");
        exit(1);
    }
    newNode->page = page;
    newNode->referenceBit = 1; //Defines reference bit to 1
    newNode->next = NULL;

    if (isSecondChanceQueueEmpty(q))
    {
        q->front = q->rear = newNode;
    }
    else
    {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    q->rear->next = q->front; // Keep circularity
}

//Dequeue page
PageTableEntry dequeueSecondChanceQueue(SecondChanceQueue *q)
{
    PageTableEntry emptyPage;
    emptyPage.valid = -1;
    if (isSecondChanceQueueEmpty(q))
    {
        printf("Erro: Fila vazia!\n");
        return emptyPage; //Error
    }

    SecondChanceQueueNode *current = q->front;

    //Search for next page to be replaced with reference bit = 0
    while (current->referenceBit == 1)
    {
        current->referenceBit = 0; //Second chance
        current = current->next;
        q->front = current;
    }

    //Removes node found with reference bit = 0
    PageTableEntry page = current->page;
    if (q->front == q->rear)
    { // Único nó na fila
        q->front = q->rear = NULL;
    }
    else
    {
        q->front = current->next;
        q->rear->next = q->front; //Keep circularity again
    }
    free(current);

    return page;
}

void freeSecondChanceQueue(SecondChanceQueue *q)
{
    SecondChanceQueueNode *temp;
    while (q->front != NULL)
    {
        temp = q->front;
        q->front = q->front->next;
        free(temp);
        if (q->front == q->rear->next)
        { 
            //Last node
            free(q->front);
            break;
        }
    }
    free(q);
}

void updateReferenceBit(SecondChanceQueue *q, PageTableEntry page)
{
    if (isSecondChanceQueueEmpty(q))
    {
        return;
    }

    SecondChanceQueueNode *current = q->front;

    do
    {
        if (current->page.frameNumber == page.frameNumber && current->page.valid == 1)
        {
            current->referenceBit = 1;
            return;
        }
        current = current->next;
    } while (current != q->front);
}