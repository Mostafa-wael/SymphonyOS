#include <stdio.h>
#include <stdlib.h>


struct Node
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
    struct Node *next;
};
typedef struct Node Node;

struct queue
{
    int count;
    Node *Head;
    Node *Tail;

};
typedef struct queue queue;

queue* createQueue() // creates a new queue
{
    queue * newQueue = malloc(sizeof(queue));   
    newQueue->count = 0;
    newQueue->Head = NULL;
    newQueue->Tail = NULL;
    return newQueue;
}

int isempty(queue *q) // check if the queue is empty
{
    return (q->Tail == NULL);
}

void enqueue(queue *q, int arrtime,int prio,int runtime,int id) // add new process to the queue
{
    
        Node *new_node;
        new_node = malloc(sizeof(Node));
        new_node->arrivaltime = arrtime;
        new_node->priority= prio;
        new_node->runningtime = runtime;
        new_node->id= id;

        new_node->next = NULL;
        if(!isempty(q))
        {
            q->Tail->next = new_node; // the new node is the next node for the tail
            q->Tail = new_node; // the new node is now the new tail
        }
        else
        {
            q->Head = q->Tail = new_node; // th head and the tail are this new node
            q->count = 0;
        }
        q->count++;
    
}



void display(Node *p)
{  
    while(p != NULL){
        printf("%d, %d, %d, %d\n",p->id,p->arrivaltime,p->runningtime , p->priority);
        p = p->next;
    }
}
