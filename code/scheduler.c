#include "headers.h"
#include "min_heap.h"
int MsgQID;
int *MsgQIDsz;
bool process_completed;
FILE *LogFile;
int RR_quanta;
struct
{
    struct proc *processes;
    int num_processes;
    int capacity;
} arrivalQ;

///////////////////////////////////////////////////////////////////////////////////////////////////
//************************************ Functions Prototypes ***********************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
void first_come_first_serve(void);
void PreemptiveHighestPriorityFirst(void);
void round_robin(void);
//
void on_msgqfull_handler(int);
void on_process_complete_awake(int);
pid_t fork_process(int);
void free_resources(int);
void recieve_proc();
///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Main loop *****************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    // message initilizations
    MsgQID = msgget(ftok("keyfile", MSGSGKEY), 0666 | IPC_CREAT);
    int MsqQIDszSHMID = shmget(ftok("keyfile", SHMSGKEY), 4, 0666 | IPC_CREAT);
    MsgQIDsz = (int *)shmat(MsqQIDszSHMID, NULL, 0);

    // initiating the arrival queue
    arrivalQ.processes = (struct proc *)malloc(MAX_NUM_PROCS * sizeof(struct proc));
    arrivalQ.capacity = MAX_NUM_PROCS;
    arrivalQ.num_processes = 0;

    // initiating signal handlers
    signal(SIGUSR1, on_msgqfull_handler);
    signal(SIGUSR2, on_process_complete_awake);
    signal(SIGINT, free_resources);

    // initiating the clock
    initClk();

    // detemine the scheduling algorithm, using function pointers
    void (*algos_ptrs[5])(void);
    algos_ptrs[0] = first_come_first_serve;
    algos_ptrs[1] = NULL;
    algos_ptrs[2] = PreemptiveHighestPriorityFirst;
    algos_ptrs[3] = NULL;
    algos_ptrs[4] = round_robin;

    // initiating the scheduler
    // open log file
    if ((LogFile = fopen("scheduler.log", "w")) == NULL)
        printf("Scheduler: Failed to open log file\n");
    else
        printf("Scheduler: managed to open log file\n");

    // get the selected algorithm
    printf("Schduler : called on %s\n", argv[0]);
    int algo_idx = atoi(argv[0]) - 1;
    if (algo_idx == 4)
    {
        RR_quanta = atoi(argv[1]);
        printf("Schduler: RoundRobin Quantum is %s\n", argv[1]);
    }

    // call the scheduling algorithm accodring to the selected one
    if (algos_ptrs[algo_idx] != NULL) // check if the algorithm was implemented or not
        algos_ptrs[algo_idx]();
    else
    {
        perror("scheduler: Called with an algorithm that wasn't implemented yet");
        exit(-1);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//************************************ Scheduling Algorithms ***********************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
void first_come_first_serve(void)
{
    while (1)
    {
        // printf("Scheduler: first_come_first_serve number of processes %d\n", arrivalQ.num_processes);
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i].id == -1)
                continue;

            fprintf(LogFile, "at %d Process with id = %d comming now \n", getClk(), arrivalQ.processes[i].id);
            fork_process(arrivalQ.processes[i].runt);

            //the sleep will not complete off course
            //but we are putting it in a while loop because it might be interrupted for a reason other
            //than the process finished
            process_completed = false;
            while (!process_completed)
                sleep(__INT_MAX__);
            fprintf(LogFile, "at %d Process %d ran to completion\n", getClk(), arrivalQ.processes[i].id);

            arrivalQ.processes[i].id = -1;
        }
    }
}
void round_robin(void)
{
    pid_t PIDS[MAX_NUM_PROCS];
    while (1)
    {
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i].id == -1)
                continue;

            process_completed = false;
            if (arrivalQ.processes[i].state == READY)
            {
                fprintf(LogFile, "at %d Process with id = %d commencing now \n", getClk(), arrivalQ.processes[i].id);

                PIDS[i] = fork_process(arrivalQ.processes[i].runt);
            }
            else
            {
                fprintf(LogFile, "at %d suspended Process with id = %d resuming now \n", getClk(), arrivalQ.processes[i].id);
                kill(PIDS[i], SIGUSR2);
            }

            int curr_time = getClk();
            while (getClk() - curr_time < RR_quanta)
            {
                if (process_completed)
                {
                    fprintf(LogFile, "\n#process %d signalled completion before quanta end#\n--\n", arrivalQ.processes[i].id);
                    break;
                }
            }

            arrivalQ.processes[i].runt -= RR_quanta;
            if (arrivalQ.processes[i].runt <= 0)
            {
                fprintf(LogFile, "at %d Process with id = %d ran to completion \n", getClk(), arrivalQ.processes[i].id);
                while (!process_completed)
                    ;
                arrivalQ.processes[i].id = -1;
            }
            else
            {
                fprintf(LogFile, "at %d Process with id = %d is suspended, Remaning time %d\n", getClk(), arrivalQ.processes[i].id, arrivalQ.processes[i].runt);

                kill(PIDS[i], SIGUSR1);
                arrivalQ.processes[i].state = SUSPENDED;
            }
        }
    }
}
void PreemptiveHighestPriorityFirst(void)
{
    while(1)
    {
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
         if (arrivalQ.processes[i].id == -1)
                continue;   
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Utilities *****************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief recieve processes as long as there are messages in the message queue.
 * this function is used to avoid polling, we use the signal as an interrupt to recieve the messages 
 * @param signum, signal number as it is a signal handler
 */
void on_msgqfull_handler(int signum)
{
    printf("Scheduler: I have received a signal that %d processes was sent\n", (*MsgQIDsz));
    for (; (*MsgQIDsz) > 0; (*MsgQIDsz)--)
        recieve_proc();

    signal(SIGUSR1, on_msgqfull_handler);
}
/**
 * @brief mark the process as complete 
 * @param signum, signal number as it is a signal handler
 */
void on_process_complete_awake(int signum)
{
    process_completed = true;
    printf("Schduler: a process has finished");
    signal(SIGUSR2, on_process_complete_awake);
}
/**
 * @brief free resources by closing the log file
 * @param signum 
 */
void free_resources(int signum)
{
    fclose(LogFile);
    printf("Schduler: I managed to close the log file\n");
    exit(0);
}
/**
 * @brief recieve new processes from the message queue, as long as the PCB table is not full,
 * in case that the PCB table is full, it ignores the recieved process.
 */
void recieve_proc()
{
    struct msgbuff reply;
    // recieve the size of the process only, not the message type-which is of type long-
    int recv = msgrcv(MsgQID, &reply, sizeof(struct msgbuff) - sizeof(long), 0, !IPC_NOWAIT);
    // check if messages are recieved properly
    if (recv == -1)
    {
        perror("Scheduler: failed to recieve the process");
    }
    else
    {
        printf("Scheduler: process was recieved successfully\n");
    }
    if (arrivalQ.num_processes < arrivalQ.capacity) // check if there is space for a new process
    {
        // process paramerters
        arrivalQ.processes[arrivalQ.num_processes].id = reply.currentProcess.id;
        arrivalQ.processes[arrivalQ.num_processes].arrt = reply.currentProcess.arrivaltime;
        arrivalQ.processes[arrivalQ.num_processes].runt = reply.currentProcess.runningtime;
        arrivalQ.processes[arrivalQ.num_processes].priorty = reply.currentProcess.priority;
        // set its state as ready
        arrivalQ.processes[arrivalQ.num_processes].state = READY;
        // inc the number of processes in the arrival queue
        arrivalQ.num_processes++;
    }
    else
        printf("Scheduler: Error recieving a process... \nPCB array is full... Process Ignored");
}
/**
 * @brief his function is responsible for running a process by froking it, it creates a new 
 * child then, use execv to start it as a new process .
 * @param runtime, indicates the remainging time for this process to finish execution
 * @return pid_t, Return -1 for errors.
 */
pid_t fork_process(int runtime)
{
    pid_t process_pid = fork();

    if (process_pid == 0) // if forking went sucessful
    {
        char runtime_as_string[12];
        sprintf(runtime_as_string, "%d", runtime);

        char *args[] = {runtime_as_string, NULL};
        while (execv("./process.out", args) == -1)
        {
            printf("Scheduler clone : Failed to create a new process... trying again");
        }
    }
    else
        return process_pid;
}
