// related to the OS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// other utilities
#include <stdio.h> //if you don't use scanf/printf change this include
#include <string.h>

// user defined libraries
#include "queues.h"

// variables and defintions
typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
#define SHMSGKEY 'S'
#define MSGSGKEY 'M'
#define SEMSGKEY 'L'

 #define MAX_NUM_PROCS 100


///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Clock *********************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////

///==============================
//don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All processes call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All processes call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/
void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//******************************************* Utilities *****************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
enum proc_state 
{
    READY,
    SUSPENDED,
    RUNNING,
    FINISHED,                                    //FINISHED
    WAITING
};
/**
 * @brief represents the PCB
 * 
 */
struct proc
{
    int id;
    int arrivalTime;
    int runningTime;
    int priority;
    int start_time;
    int remaining_time;
    int wait_time;
    int mem;
    enum proc_state state;
};
typedef struct proc proc;

///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Message Queue *************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief the message format-> mtype and a process
 * 
 */
struct msgbuff 
{
    long mtype;
    proc currentProcess;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//********************************************* Min Heap ***************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
#define leftChild(i) (2 * i + 1)
#define rightChild(i) (2 * i + 2)
#define parent(i) ((i - 1) / 2)

/**
 * @brief the heap node which is a process and its key
 * 
 */
struct heap_node
{   
    proc* process;
    int key;
};
typedef struct heap_node heap_node;

/**
 * @brief a min heap of heap nodes
 * 
 * 
 */
struct min_heap
{
    heap_node**heap;
    int size;
    int capacity;
};
typedef struct min_heap min_heap;

/**
 * @brief insert a heap node to the heap
 * 
 * @param m pointer to the min heap (priority queue)
 * @param n the heap node which is a process and its key
 */
void min_heap_insert(min_heap *m, heap_node *n)
{
    if (m->size >= m->capacity)
    {
        return;
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
/**
 * @brief to reorder the heap after extracting a heap node from it
 * 
 * @param m pointer to the min heap (priority queue)
 * @param i index
 */
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

/**
 * @brief extract the top node and removes it from the heap
 * 
 * @param m pointer to the min heap (priority queue)
 * @return heap_node* 
 */
heap_node* min_heap_extract(min_heap* m) // it affects the size of the heap, don't use it in a for loop depending on the size of the heap
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//******************************************* Semaphores ***************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};
/**
 * @brief Get the Semaphore Value 
 * 
 * @param semSetID 
 * @param semIdx 
 * @return int 
 */

int getSemaphoreValue(int semSetID, int semIdx)
{
    return semctl(semSetID,semIdx,GETVAL);
}
/**
 * @brief clears the valu of a semaphore, sets it to 0
 * 
 * @param semSetID 
 * @param semIdx 
 * @return int 
 */
int clearSemaphoreValue(int semSetID,int semIdx)
{
    union Semun semun ;
    semun.val = 0;
    return semctl(semSetID, semIdx, SETVAL, semun);
}

/**
 * @brief sets the valu of a semaphore, sets it to 1
 * 
 * @param semSetID 
 * @param semIdx 
 * @return int 
 */
int setSemaphoreValue(int semSetID, int semIdx)
{
    union Semun semun ;
    semun.val = 1;
    return semctl(semSetID, semIdx, SETVAL, semun);
}


//----------------------------------memory--------------------------

//*************************************************free memory list*******************************
#define MEM_SIZE 128
#define MIN_SIZE_BUDDY 8  

enum node_type {PROC,HOLE};

struct mem_node{
    int start ;
    int length  ;
    enum node_type type ;

    struct mem_node* next ;

    struct mem_node* buddy ;
};

struct freemem_list_t {
    struct mem_node* head ;

    struct mem_node* next_fit_ptr ;

    int total_free ;
};