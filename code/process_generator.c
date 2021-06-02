#include "headers.h"

void clearResources(int);
int MsgQID;


int main(int argc, char *argv[]) // file name, scheduling algorithm
{
    signal(SIGINT, clearResources);
    // test the passed arguments
    printf("\nNumber of passed arguments to the process_generator is %d\n", argc);
    for (int i = 1; i < argc; i++)
        printf("passed argument number %d is: %s\n", i, argv[i]);
    //****************************************************************
    // 1. Read the input files and save them
    char *FileName = argv[1];
    FILE *pFile;
    pFile = fopen(FileName, "r"); // read mode
    if (pFile == NULL)
    {
        perror("\nError while opening the file:");
        exit(EXIT_FAILURE);
    }
    //******************************
   // int tableSize = 100;                                             // indicates the max arrival time
   // comingProcess **procTable = createComingProcessTable(tableSize); // creates a table (hashmap) for the commingProcesses
    char buff[50]; 
    unsigned int number_processes = 0; 
    while (fgets(buff, sizeof(buff), pFile) != NULL)
    {
        if (buff[0] != '#')
            number_processes++;

    }
    //check 
    printf("%d\n",number_processes);

    // return to beginning of the file again
    fseek( pFile, 0, SEEK_SET );
    
    // create array of processes 
    comingProcess * procTable = (struct comingProcess *) malloc(sizeof(struct comingProcess) * number_processes);
     
    unsigned int index = 0;      
                                              
    while (fgets(buff, sizeof(buff), pFile) != NULL)
    {
        if (buff[0] == '#')
            continue;

        sscanf(buff, "%d\t%d\t%d\t%d",
               &(procTable[index].id),
               &(procTable[index].arrivaltime),
               &(procTable[index].runningtime),
               &(procTable[index].priority));

        index++;
    }
    fclose(pFile);
    // print the processes
    printf("#id arrival runtime priority\n");

    //check
    for (int i = 0; i < number_processes; i++)
    {
        printf("%d , %d ,%d , %d \n",procTable[i].id,procTable[i].arrivaltime,procTable[i].runningtime,procTable[i].priority);
    }
    
    //****************************************************************
    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    char *chosen_algorithm = argv[2];
    char *additionalParameters = argv[3]; // like quantum in case of RR
    printf("%s", chosen_algorithm);
    // 3. Initiate and create the scheduler and clock processes.
    int pidSch = fork();
    if (pidSch == 0)
    {
        printf("ProcessGenerator : I have started the scheduler");
        char *arr[] = {chosen_algorithm, additionalParameters, NULL};
        execv("./scheduler.out", arr);

    }
    int pidCLK = fork();
    if (pidCLK == 0)
    {
        printf("ProcessGenerator : I have started the clock");
        char *arr[] = {NULL};
        execv("./clk.out", arr);
    }
    // create keys for the message queue
    key_t KeyID = ftok("keyfile", SHMSGKEY); // the key of the Queue
    // get the message queues
    MsgQID = msgget(KeyID, 0666 | IPC_CREAT); //create messageReceived queue and return id
    // check for any errors
    if (MsgQID == -1) // if messageid == -1, it failed to create the queue
    {
        perror("Error in create");
        exit(-1);
    }

    // 4. Use this function after creating the clock process to initialize clock.
    initClk();
    // TODO Generation Main Loop
    //****************************************************************
    // 5. Create a data structure for processes and provide it with its parameters.
    // done while reading the file
    //****************************************************************
    // 6. Send the information to the scheduler at the appropriate time.
    struct msgbuff currentProcesses;
    // shared memory 4 byte and id ftok keyfile 'M'
    // int * msgqueue    
    int prevTime = getClk();
    unsigned int current_index = 0;
    //procTable[number_processes-1].arrivaltime > getClk()
    
    while (1)
    {
        int currentTime = getClk();
        if (currentTime != prevTime) // add this condition to avoid printing the time a lot
        {
            printf("ProcessGenerator : Current Time is %d\n", currentTime);
            while (procTable[current_index].arrivaltime == currentTime)
            {
                printf("ProcessGenerator : There is a process at Time %d\n", currentTime);

                    // send the value to the server
                    currentProcesses.mtype = 7;
                    currentProcesses.currentProcess = procTable[current_index];
                    int sendValue = msgsnd(MsgQID, &currentProcesses, sizeof(currentProcesses.currentProcess), !IPC_NOWAIT);
                    if (sendValue == -1)
                        perror("Errror in send");
                    else{
                        //message queue size ++ 
                        printf("ProcessGenerator : %d, %d, %d, %d\n",
                           procTable[current_index].id,
                           procTable[current_index].arrivaltime,
                           procTable[current_index].runningtime,
                           procTable[current_index].priority);
                    }    
                    current_index++;
            }
            //signusr1(shr);
            prevTime = currentTime;
        }
    }
    // 
    //
    //
    //****************************************************************s
    // 7. Clear clock resources
    destroyClk(true);
    
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    // remove the message queue from the kernel
    msgctl(MsgQID, IPC_RMID, (struct msqid_ds *)0);
    // kill all the processes
    killpg(getpgrp(), SIGKILL);
    // re-set the signal hnadler
    signal(SIGINT, clearResources);
}
