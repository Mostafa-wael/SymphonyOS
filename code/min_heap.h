#include "headers.h"

struct min_heap
{
    proc* heap[MAX_NUM_PROCS];
    int size;
};
typedef struct min_heap min_heap;


int leftChild(int i)
{
    return 2 * i + 1;
}

int rightChild(int i)
{
    return 2 * i + 2;
}

int parent(int i)
{
    return (i - 1) / 2;
}

void min_heap_insert(min_heap *m, proc *p)
{
    if (m->size == MAX_NUM_PROCS)
    {
        //error
    }
    int index = m->size;
    m->size++;
    m->heap[index] = p;

    while (index != 0 && m->heap[parent(index)]->runt > m->heap[index]->runt)
    {
        proc *temp = m->heap[index];
        m->heap[index] = m->heap[parent(index)];
        m->heap[parent(index)] = temp;
        index = parent(index);
    }
}

void min_heapify(min_heap* m, int i)
{
    int left = leftChild(i);
    int right = rightChild(i);
    int min = i;

    if (min < m->size && m->heap[left]->runt < m->heap[min]->runt)
    {
        min = left;
    }
    if (right < m->size && m->heap[right]->runt < m->heap[min]->runt)
    {
        min = right;
    }
    if (min != i)
    {
        proc *temp = m->heap[i];
        m->heap[i] = m->heap[min];
        m->heap[min] = temp;
        min_heapify(m, min);
    }
}

proc* min_heap_extract(min_heap *m)
{
    if (m->size == 1)
    {
        m->size = 0;
        return m->heap[0];
    }

    proc *root = m->heap[0];
    m->heap[0] = m->heap[m->size - 1];
    m->size--;
    min_heapify(m, 0);
    return root;
}





//test main
/*
int main()
{
    min_heap m;
    m.size = 0;

    // struct proc p1;
    // p1.runt = 7;

    // struct proc p2;
    // p2.runt = 3;

    // insert_min_heap(&m, &p1);
    // insert_min_heap(&m, &p2);

    // printf("%d\n%d\n", m.heap[0]->runt, m.heap[1]->runt);

    // int x = extract_min(&m)->runt;
    // printf("%d\n", x);

    int temp[] = {1, 5, 7, 2, 6, 0, 9, 3, 12, 10, 15, 13};
    proc p[12];
    for (int i = 11; i >= 0; i--)
    {
        p[i].runt = temp[i];
        min_heap_insert(&m, &p[i]);
    }

    for (int i = 0; i <= 11; i++)
    {
        printf("%d ", m.heap[i]->runt);
    }
    printf("\n");
    printf("%d\n", min_heap_extract(&m)->runt);
    for (int i = 0; i <= 11; i++)
    {
        printf("%d ", m.heap[i]->runt);
    }
    printf("\n");
    return 0;

}
*/
