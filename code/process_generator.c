#include "headers.h"
// typedef struct
// {
//     int arrivaltime;
//     int priority;
//     int runningtime;
//     int id;
// } processData;
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
    char * FileName = argv[1];
    //printf("%s\n",FileName);
    FILE *pFile;
    pFile = fopen(FileName, "r"); // read mode // TODO: add it as a variable
    if (pFile == NULL)
    {
        perror("\nError while opening the file:");
        exit(EXIT_FAILURE);
    }
    //******************************
    printf("#id arrival runtime priority\n");

    // only test queue
    queue * ready_queue;
    ready_queue = malloc(sizeof(queue));
    initialize(ready_queue);

    char buff[150];
    int processCount = 0;
    
    int count  = 0;
    int id,arrivaltime,runningtime,priority;

    while (fgets(buff, sizeof(buff), pFile) != NULL)
    {
        if (buff[0] == '#'){
            continue;
        }
        sscanf(buff,"%d\t%d\t%d\t%d",
               &(id),
               &(arrivaltime),
               &(runningtime),
               &(priority));
        // put current process in ready queue;
        enqueue(ready_queue,arrivaltime,priority,runningtime,id);
    }
    
    fclose(pFile);
    
    //check
    printf("the number of processes is :%d \n",ready_queue->count);
    display(ready_queue->Head);

    //****************************************************************
    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    int chosen_algorithm = atoi(argv[2]);
    printf("%d",chosen_algorithm);
    // 3. Initiate and create the scheduler and clock processes.


    
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
