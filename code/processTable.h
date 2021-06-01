//************************************************* Coming processes Table **************************************************//
// coming processes table as an array of comingProcesss to achieve O(1 + m) retrieval time.
// every entity in the array is a linked list
#include <stdio.h>
#include <stdlib.h>

struct comingProcess // this noew represents the process read by the process generator
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
    struct comingProcess *next;
};
typedef struct comingProcess comingProcess;

comingProcess *createComingProcess(int arrtime, int prio, int runtime, int id)
{
    comingProcess *new_process;
    new_process = malloc(sizeof(comingProcess));
    new_process->arrivaltime = arrtime;
    new_process->priority = prio;
    new_process->runningtime = runtime;
    new_process->id = id;

    new_process->next = NULL;

    return new_process;
}

comingProcess **createComingProcessTable(int initialSize) // array of pointers to comingProcess
{
    comingProcess **table = (comingProcess **)calloc(initialSize + 1, sizeof(comingProcess *));
    return table;
}

void addComingProcess(comingProcess **procTable, comingProcess *proc) // array of comingProcesses, comingProcess
{
    if (procTable[proc->arrivaltime] == NULL) // create a new slot only if nedded
    {
        procTable[proc->arrivaltime] = createComingProcess(0, 0, 0, 0);
    }

    comingProcess *p = procTable[proc->arrivaltime];
    while (p->next != NULL)
    {
        p = p->next;
    }
    p->next = proc;
}

void showComingProcesses(comingProcess **procTable, int tableSize)
{
    printf("Size =  %d\n", tableSize);
    for (int i = 0; i < tableSize; i++)
    {
        if (procTable[i] != NULL)
        {
            printf("at time %d\n", i);
            comingProcess *p = procTable[i];
            while (p->next != NULL)
            {
                p = p->next;
                printf("%d, %d, %d, %d\n", p->id, p->arrivaltime, p->runningtime, p->priority);
            }
        }
    }
}
