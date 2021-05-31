#include "headers.h"
typedef struct
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
} processData;
void clearResources(int);

int main(int argc, char *argv[]) // file name, scheduling algorithm
{
    signal(SIGINT, clearResources);
    // test the passed arguments
    printf("\nNumber of passed arguments to the process_generator is %d\n", argc);
    for (int i = 1; i < argc; i++)
        printf("passed argument number %d is: %s\n", i, argv[i]);
    //****************************************************************
    // 1. Read the input files and save them
    // const char* file_name = argv[2];
    FILE *pFile;
    pFile = fopen("processes.txt", "r"); // read mode // TODO: add it as a variable
    if (pFile == NULL)
    {
        perror("\nError while opening the file:");
        exit(EXIT_FAILURE);
    }
    //******************************
    int numProcesses = 10; // TODO: check for the number of processes
    printf("we have %d processes, as follows:\n", numProcesses);
    printf("#id arrival runtime priority\n");
    processData *processes = malloc(sizeof(processData) * numProcesses);
    char buff[10];
    int processCount = 0;
    while (fgets(buff, 10, pFile) != NULL)
    {
        if (buff[0] == '#')
            continue;
        // TODO: proper reading of the file
        printf("%s\n", buff);
        sscanf(buff, "%d\t%d\t%d\t%d",
               &(processes[processCount].id),
               &(processes[processCount].arrivaltime),
               &(processes[processCount].runningtime),
               &(processes[processCount].priority));

        printf("%d, %d, %d, %d",
               processes[processCount].id,
               processes[processCount].arrivaltime,
               processes[processCount].runningtime,
               processes[processCount].priority);
        processCount++;
    }

    fclose(pFile);
    //****************************************************************
    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    //  1.FirstComeFirstServe(FCFS)
    //  2.ShortestJobFirst(SJF)
    //  3.PreemptiveHighestPriorityFirst(HPF)
    //  4.ShortestRemainingTimeNext(SRTN)
    //  5.RoundRobin(RR)
    int schedulingAlgorithm = atoi(argv[2]);
    printf("The chosen scheduling algorithm is: %d\n", schedulingAlgorithm);
    int quantum = 0;
    if (schedulingAlgorithm == 5) // in case of RoundRobin, get the value of the required quantum from the user
    {
        quantum = atoi(argv[3]);
        printf("With a quantum = %d\n", quantum);
    }
    //****************************************************************
    //****************************************************************
    // 3. Initiate and create the scheduler and clock processes.

    //****************************************************************
    // 4. Use this function after creating the clock process to initialize clock.
    // initClk();
    // // To get time use this function.
    // int x = getClk();
    // printf("Current Time is %d\n", x);
    // TODO Generation Main Loop
    //****************************************************************
    // 5. Create a data structure for processes and provide it with its parameters.
    //****************************************************************
    // 6. Send the information to the scheduler at the appropriate time.
    //****************************************************************
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    killpg(getpgrp(), SIGKILL);
}
