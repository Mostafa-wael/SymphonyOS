#include "headers.h"

// this file is responsible for simulating the process as if it is running
int remainingtime;    // until the process finishes
bool process_running; // runnung or not
int process_interrupt_semaphores ; //scheduler writes to this set to set process_running
///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Main loop *****************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
int main(int agrc, char *argv[])
{
    initClk();

    remainingtime = atoi(argv[0]); // take the remaining time as an argumnet 
    int id = atoi(argv[1]);
   
    process_running = true;

    process_interrupt_semaphores = semget(ftok("keyfile",SEMSGKEY),100,0444|IPC_CREAT);
    FILE* DEBUG = fopen("proclog.log","a");
    int prev = getClk();
    while (remainingtime > 0) // decremnt the remaining time every one clock tick
    {
        int curr = getClk();

        if (prev != curr)
        {
            process_running = getSemaphoreValue(process_interrupt_semaphores,id);

            if (process_running)
                remainingtime--;
            
            fprintf(DEBUG,"\n---P%d : at %d rem %d PR=%d\n---",id,getClk(),remainingtime,process_running);
            
            prev = curr;
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
