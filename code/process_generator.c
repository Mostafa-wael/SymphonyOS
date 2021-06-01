#include "headers.h"
#include "processTable.h"
void clearResources(int);
int MsgQID;
struct msgbuff // the message format
{
    long mtype;
    comingProcess *currentProcess;
};

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
    int tableSize = 100;                                             // indicates the max arrival time
    comingProcess **procTable = createComingProcessTable(tableSize); // creates a table (hashmap) for the commingProcesses
    char buff[50];                                                   // line length
    while (fgets(buff, sizeof(buff), pFile) != NULL)
    {
        if (buff[0] == '#')
            continue;

        int id, arrivaltime, runningtime, priority;
        sscanf(buff, "%d\t%d\t%d\t%d",
               &(id),
               &(arrivaltime),
               &(runningtime),
               &(priority));

        // create a new comingProcess and add it to the table
        comingProcess *currentProcess = createComingProcess(arrivaltime, priority, runningtime, id);
        addComingProcess(procTable, currentProcess);
    }
    fclose(pFile);
    // print the processes
    printf("#id arrival runtime priority\n");
    showComingProcesses(procTable, tableSize);
    //****************************************************************
    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    char *chosen_algorithm = argv[2];
    char *additionalParameters = argv[3]; // like quantum in case of RR
    printf("%s", chosen_algorithm);
    // 3. Initiate and create the scheduler and clock processes.
    int pid = fork();
    if (pid == 0)
    {
        char *arr[] = {NULL};
        execv("./scheduler.out", arr);
        // execl("./clock.out", NULL);
    }
    // create keys for the message queue
    key_t KeyID = ftok("keyfile", 'S'); // the key of the Queue
    // get the message queues
    MsgQID = msgget(KeyID, 0666 | IPC_CREAT); //create messageReceived queue and return id
    // check for any errors
    if (MsgQID == -1) // if messageid == -1, it failed to create the queue
    {
        perror("Error in create");
        exit(-1);
    }
    pid = fork();
    if (pid == 0)
    {
        char *arr[] = {chosen_algorithm, additionalParameters, NULL};
        execv("./scheduler.out", arr);
        // execl("./scheduler.out", chosen_algorithm, additionalParameters, NULL);
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
    while (1)
    {
        int currentTime = getClk();
        printf("Current Time is %d\n", currentTime);
        if (procTable[currentTime] != NULL)
        {
            comingProcess *currentProcess = procTable[currentTime];
            while (currentProcess->next != NULL)
            {
                currentProcess = currentProcess->next;

                printf("%d, %d, %d, %d\n",
                       currentProcess->id,
                       currentProcess->arrivaltime,
                       currentProcess->runningtime,
                       currentProcess->priority);
                currentProcesses.currentProcess = currentProcess;
                // send the value to the server
                int sendValue = msgsnd(MsgQID, &currentProcesses, sizeof(currentProcesses.currentProcess), !IPC_NOWAIT);

                if (sendValue == -1)
                    perror("Errror in send");
            }
            printf("I have sent all the processes at time %d\n", currentTime);
        }
    }

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
