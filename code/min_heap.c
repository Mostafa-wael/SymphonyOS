#include "headers.h"


struct heap_node
{
    proc* process;
    int key;
};
typedef struct heap_node heap_node;


struct min_heap
{
    heap_node* heap[MAX_NUM_PROCS];
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

void min_heap_insert(min_heap *m, heap_node *n)
{
    if (m->size == MAX_NUM_PROCS)
    {
        //error
    }
    int index = m->size;
    m->size++;
    m->heap[index] = n;

    while (index != 0 && m->heap[parent(index)]->key > m->heap[index]->key)
    {
        heap_node* temp = m->heap[index];
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

    if (left < m->size && min < m->size && m->heap[left]->key < m->heap[min]->key)
    {
        min = left;
    }
    if (right < m->size && right < m->size && m->heap[right]->key < m->heap[min]->key)
    {
        min = right;
    }
    if (min != i)
    {
        heap_node* temp = m->heap[i];
        m->heap[i] = m->heap[min];
        m->heap[min] = temp;
        min_heapify(m, min);
    }
}

heap_node* min_heap_extract(min_heap* m)
{
    if (m->size == 1)
    {
        m->size = 0;
        return m->heap[0];
    }

    heap_node* root = m->heap[0];
    m->heap[0] = m->heap[m->size - 1];
    m->size--;
    min_heapify(m, 0);
    return root;
}



//test main

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

    int temp[] = {1,1,1,1,1,1,1,5,1,0,1,1};
    heap_node n[12];
    proc p[12];
    for (int i = 0; i < 12; i++)
    {
        p[i].runt = temp[i];
        p[i].id = i;
        n[i].process = &p[i];
        n[i].key = n[i].process->runt;
        min_heap_insert(&m, &n[i]);
    }

    for (int i = 0; i < m.size; i++)
    {
        printf("%d ", m.heap[i]->process->runt);
    }
    printf("\n");
    printf("%d\n", min_heap_extract(&m)->process->runt);
    for (int i = 0; i < m.size; i++)
    {
        printf("%d ", m.heap[i]->process->runt);
    }
    printf("\n");
    return 0;

}
