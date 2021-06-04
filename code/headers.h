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
#include "processTable.h"
// variables and defintions
typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
#define SHMSGKEY 'S'

#define MAX_NUM_PROCS 100

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

//**************************************************************** Message Queues ****************************************************************//
struct msgbuff // the message format
{
    long mtype;
    comingProcess currentProcess;
};

enum proc_state 
{
    READY,
    SUSPENDED
};
struct proc
{
    int id;
    int arrt;
    int runt;
    int priorty;
    enum proc_state state;
};
typedef struct proc proc;
