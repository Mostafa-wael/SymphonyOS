#include "headers.h"

//test main, make the file.c instaed of file.h

int main()
{
    min_heap m;
    m.size = 0;

    // struct proc p1;
    // p1.runningTime = 7;

    // struct proc p2;
    // p2.runningTime = 3;

    // insert_min_heap(&m, &p1);
    // insert_min_heap(&m, &p2);

    // printf("%d\n%d\n", m.heap[0]->runningTime, m.heap[1]->runningTime);

    // int x = extract_min(&m)->runningTime;
    // printf("%d\n", x);

    int temp[] = {1,1,3,1,7,2,1,5,1,0,1,1}; // 12
    proc p[12];
    heap_node n[12]; // consists of a proc and a key
    for (int i = 0; i < 12; i++) // inserting nodes to the heap
    {
        p[i].runningTime = temp[i];
        p[i].id = i;
        //
        n[i].process = &p[i];
        n[i].key = n[i].process->runningTime; // here the key is the runtime
        min_heap_insert(&m, &n[i]);
    }

    for (int i = 0; i < m.size; i++)
    {
        printf("%d ", m.heap[i]->process->runningTime);
    }
    printf("\n");
    for (int i = 0; i < m.size; i++)
    {
        printf("%d ", m.heap[i]->process->runningTime);
    }
    printf("\n");
    while (m.size)
    {
        printf("%d ", min_heap_extract(&m)->process->runningTime);
    }

    printf("\n");
    return 0;

}
