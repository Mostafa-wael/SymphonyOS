#include "headers.h"
int MsgQID;
int main(int argc, char *argv[])
{
    initClk();
    
   
    printf("scheduler: Algorithm: %s \n", argv[1]);
            // create keys for the message queue
        key_t KeyID = ftok("keyfile", SHMSGKEY); // the key of the Queue
        // get the message queues
        MsgQID = msgget(KeyID, 0666 | IPC_CREAT); //create messageReceived queue and return id
        // check for any errors
        if (MsgQID == -1) // if messageid == -1, it failed to create the queue
        {
            perror("scheduler: Error in create");
            exit(-1);
            
        }
         struct msgbuff currentProcesses;
    while (1)
    {

       
        int recValue = msgrcv(MsgQID, &currentProcesses, sizeof(currentProcesses.currentProcess), 7, !IPC_NOWAIT);

        if (recValue == -1)
            perror("scheduler: Error in receive");
        else
        {
             printf("\nMessage received:\n");
            printf("scheduler: %d, %d, %d, %d\n \n",
                   currentProcesses.currentProcess.id,
                   currentProcesses.currentProcess.arrivaltime,
                   currentProcesses.currentProcess.runningtime,
                   currentProcesses.currentProcess.priority);
        }

        //TODO: implement the scheduler.
        //TODO: upon termination release the clock resources.
    }
    destroyClk(true);
}
