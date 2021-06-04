#include "headers.h"

// this file is responsible for simulating the process as if it is running
int remainingtime;    // until the process finishes
bool process_running; // runnung or not

void on_interruption_sleep(int);
void on_resumption_awaken(int);

///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Main loop *****************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
int main(int agrc, char *argv[])
{
    initClk();

    remainingtime = atoi(argv[0]); // take the remaining time as an argumnet 

    signal(SIGUSR1, on_interruption_sleep); // make the process sleep if it has recieved a sleeping signal 
    signal(SIGUSR2, on_resumption_awaken); // awaking the process if it has recieved an awaken signal
    process_running = true;

    int prev = getClk();
    while (remainingtime > 0) // decremnt the remaining time every one clock tick
    {
        int curr = getClk();

        if (prev != curr)
        {
            if (process_running)
                remainingtime--;

            prev = curr;
        }
    }
    kill(getppid(), SIGUSR2); // mark the process as complete when its remaining time is zero!
    printf("Process: a process has finished");


    destroyClk(false);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Utilities *****************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
void on_interruption_sleep(int signum) // make the process sleep if it has recieved a sleeping signal 
{
    process_running = false;
    signal(SIGUSR1, on_interruption_sleep);
}
void on_resumption_awaken(int signum) // awaking the process if it has recieved an awaken signal
{
    process_running = true;
    signal(SIGUSR2, on_resumption_awaken);
}