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

void initialize(queue *q)
{
    q->count = 0;
    q->Head = NULL;
    q->Tail = NULL;
}

int isempty(queue *q)
{
    return (q->Tail == NULL);
}

void enqueue(queue *q, int arrtime,int prio,int runtime,int id)
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
            q->Tail->next = new_node;
            q->Tail = new_node;
        }
        else
        {
            q->Head = q->Tail = new_node;
        }
        q->count++;
    
}



void display(Node *p)
{  
    while(p != NULL){
        printf("%d , %d , %d , %d \n",p->id,p->arrivaltime,p->runningtime , p->priority);
        p = p->next;
    }
}
