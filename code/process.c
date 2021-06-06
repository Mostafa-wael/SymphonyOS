#include "headers.h"

// this file is responsible for simulating the process as if it is running
int remainingtime;    // until the process finishes
int process_interrupt_shmid; //scheduler writes to this set to set process_running
int process_remaining_shmid; //process writes to this to inform the scheduler of it's remaining time
char* process_interrupt_flags ;
char* process_remaining_flags ;
///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Main loop *****************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
int main(int agrc, char *argv[])
{
    initClk();

    remainingtime = atoi(argv[0]); // take the remaining time as an argumnet 
    int id = atoi(argv[1]);

    process_interrupt_shmid = shmget(ftok("keyfile",'Y'),MAX_NUM_PROCS,0666|IPC_CREAT);
    process_remaining_shmid = shmget(ftok("keyfile",'R'),MAX_NUM_PROCS,0666|IPC_CREAT);
    process_interrupt_flags = (char*)shmat(process_interrupt_shmid,NULL,0);
    process_remaining_flags = (char*)shmat(process_remaining_shmid,NULL,0);

    FILE* DEBUG = fopen("proclog.log","a");

    bool process_stopped = false ;
    while (remainingtime > 0) // decremnt the remaining time every one clock tick
    {
        int curr = getClk();
        fprintf(DEBUG,"P%d: at %d currently running, rem %d\n---\n",id,curr,remainingtime);

        while(curr == getClk()) {
            process_stopped = *(process_interrupt_flags+id);
            if(process_stopped) break;
        }
        remainingtime--;
        *(process_remaining_flags+id) = remainingtime;
        if(remainingtime == 0) break ;

        for(int i = 0; i<10000; i++) process_stopped = *(process_interrupt_flags+id);
        if(process_stopped = *(process_interrupt_flags+id)) {
            fprintf(DEBUG,"P%d: at %d stopped, rem %d\n---\n",id,getClk(),remainingtime);

            raise(SIGSTOP);

            while(process_stopped = *(process_interrupt_flags+id));
        }
    }
    fclose(DEBUG);
    kill(getppid(), SIGUSR2); // mark the process as complete when its remaining time is zero!
    printf("Process: a process has finished\n");

    destroyClk(false);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Utilities *****************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
