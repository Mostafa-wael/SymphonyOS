#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
bool process_running ;

void on_interruption_sleep(int);
void on_resumption_awaken(int);
int main(int agrc, char *argv[])
{
    initClk();

    remainingtime = atoi(argv[0]);

    signal(SIGUSR1,on_interruption_sleep);
    signal(SIGUSR2,on_resumption_awaken);
    process_running = true ;
    int prev = getClk();
    while (remainingtime > 0)
    {        
        int curr = getClk();

        if(prev != curr){
            if(process_running) remainingtime-- ;
            
            prev = curr ;
        }
    }

    kill(getppid(),SIGUSR2);
    
    destroyClk(false);

    return 0;
}

void on_interruption_sleep(int signum){
    process_running = false ;
    signal(SIGUSR1,on_interruption_sleep);
}
void on_resumption_awaken(int signum){
    process_running = true ;
    signal(SIGUSR2,on_resumption_awaken);
}