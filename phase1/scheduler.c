#include "headers.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//************************************ #defines ***********************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
#define AT_TIME__ "At\ttime\t%d\t"
#define PROC__ "process\t%d\t%s\t"
#define ARR__ "arr\t%d\t"
#define TOTAL__ "total\t%d\t"
#define REMAIN__ "remain\t%d\t"
#define WAIT__ "wait\t%d"
#define SCHEDULER_LOG_NON_FINISH_LINE_FORMAT AT_TIME__ PROC__ "\t" ARR__ TOTAL__ REMAIN__ WAIT__ "\n"
#define SCHEDULER_LOG_FINISH_LINE_FORMAT AT_TIME__ PROC__ ARR__ TOTAL__ REMAIN__ WAIT__ "\tTA\t%d" \
                                                                                        "\tWTA\t%.2f\n"

///////////////////////////////////////////////////////////////////////////////////////////////////
//************************************ globals***********************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
int MsgQID;
int *MsgQIDsz;
int process_interrupt_shmid; //scheduler writes to this set to set process_running
int process_remaining_shmid; //process writes to this to inform the scheduler of it's remaining time
char *process_interrupt_flags;
char *process_remaining_flags;

int RR_quanta;
bool process_completed;

FILE *LogFile;
int total_wait = 0;
float total_WTA = 0.0;
int max_num_processes;

struct
{
    struct proc *processes[MAX_NUM_PROCS];
    int num_processes;
    int capacity;
} arrivalQ;

///////////////////////////////////////////////////////////////////////////////////////////////////
//************************************ Functions Prototypes ***********************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
void first_come_first_serve(void);
void shortest_job_first(void);
void highest_priority_first(void);
void shortest_remaining_time_next(void);
void round_robin(void);

void on_msgqfull_handler(int);
void on_process_complete_awake(int);

pid_t fork_process(int, int);
void free_resources(int);
void recieve_process();

///////////////////////////////////////////////////////////////////////////////////////////////////
//****************************************** Main function *************************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    // message initilizations
    MsgQID = msgget(ftok("keyfile", MSGSGKEY), 0666 | IPC_CREAT);
    int MsqQIDszSHMID = shmget(ftok("keyfile", SHMSGKEY), 4, 0666 | IPC_CREAT);
    MsgQIDsz = (int *)shmat(MsqQIDszSHMID, NULL, 0);

    process_interrupt_shmid = shmget(ftok("keyfile", 'Y'), MAX_NUM_PROCS, 0666 | IPC_CREAT);
    process_remaining_shmid = shmget(ftok("keyfile", 'R'), MAX_NUM_PROCS, 0666 | IPC_CREAT);
    process_interrupt_flags = (char *)shmat(process_interrupt_shmid, NULL, 0);
    process_remaining_flags = (char *)shmat(process_remaining_shmid, NULL, 0);

    // initiating the arrival queue
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
    algos_ptrs[1] = shortest_job_first;
    algos_ptrs[2] = highest_priority_first;
    algos_ptrs[3] = shortest_remaining_time_next;
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
    max_num_processes = atoi(argv[2]);

    if (algo_idx == 4)
    {
        RR_quanta = atoi(argv[1]);
        printf("Schduler: RoundRobin Quantum is %s\n", argv[1]);
    }

    algos_ptrs[algo_idx]();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//************************************ Scheduling Algorithms ***********************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
void first_come_first_serve(void)
{
    while (1)
    {
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i]->state != READY)
                continue;

            int wait = getClk() - arrivalQ.processes[i]->arrivalTime;
            fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                    getClk(),                            //At time
                    arrivalQ.processes[i]->id, "started", //process started
                    arrivalQ.processes[i]->arrivalTime,          //arrival
                    arrivalQ.processes[i]->runningTime,          //total
                    arrivalQ.processes[i]->runningTime,          //remain
                    wait                                 //wait
            );
            fork_process(arrivalQ.processes[i]->runningTime, arrivalQ.processes[i]->id);

            //the sleep will not complete off course
            //but we are putting it in a while loop because it might be interrupted for a reason other
            //than the process finished
            process_completed = false;
            while (!process_completed)
                sleep(__INT_MAX__);

            int TA = getClk() - arrivalQ.processes[i]->arrivalTime;
            float WTA = ((float)TA) / ((float)arrivalQ.processes[i]->runningTime);
            fprintf(LogFile, SCHEDULER_LOG_FINISH_LINE_FORMAT,
                    getClk(),                             //At time
                    arrivalQ.processes[i]->id, "finished", //process started
                    arrivalQ.processes[i]->arrivalTime,           //arrival
                    arrivalQ.processes[i]->runningTime,           //total
                    0,                                    //remain
                    wait,                                 //wait
                    TA,                                   //TA
                    WTA                                   //WTA
            );

            arrivalQ.processes[i]->state = FINISHED;
            total_wait += wait;
            total_WTA += WTA;
        }
        if (arrivalQ.num_processes == max_num_processes)
            kill(getppid(), SIGINT);
    }
}

void shortest_job_first(void)
{
    min_heap priority_queue;
    priority_queue.size = 0;
    int executed_processes = 0;

    while (true)
    {
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i]->state != READY)
            {
                continue;
            }

            //creating a new heap node and setting its priority to running time (shortest running time first)
            heap_node *node = (heap_node *)malloc(sizeof(heap_node));
            node->process = arrivalQ.processes[i];
            node->key = node->process->runningTime;
            node->process->state = RUNNING;
            min_heap_insert(&priority_queue, node);
        }
        if (priority_queue.size > 0) //there are processes to be scheduled
        {
            heap_node* running_node = min_heap_extract(&priority_queue);

            int wait = getClk() - running_node->process->arrivalTime;
            fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                    getClk(),               //At time
                    running_node->process->id, "started", //process started
                    running_node->process->arrivalTime,          //arrival
                    running_node->process->runningTime,          //total
                    running_node->process->runningTime,          //remain
                    wait                    //wait
            );

            fork_process(running_node->process->runningTime, running_node->process->id);
            process_completed = false;
            while (!process_completed) //wait for process to be completed
            {
                sleep(__INT_MAX__);
            }
            executed_processes++;
            running_node->process->state = FINISHED;

            int TA = getClk() - running_node->process->arrivalTime;
            float WTA = ((float)TA) / ((float)running_node->process->runningTime);
            total_wait += wait;
            total_WTA += WTA;

            fprintf(LogFile, SCHEDULER_LOG_FINISH_LINE_FORMAT,
                    getClk(),                //At time
                    running_node->process->id, "finished", //process started
                    running_node->process->arrivalTime,           //arrival
                    running_node->process->runningTime,           //total
                    0,                       //remain
                    wait,                    //wait
                    TA,                      //TA
                    WTA                      //WTA
            );
        }
        if (executed_processes == max_num_processes)
        {
            kill(getppid(), SIGINT);
        }
    }
}

void highest_priority_first(void)
{
    /* 4 cases
    * 1) there is no running process, priority_queue is empty -> do nothing
    * 2) there is a running process, priority_queue is empty -> do nothing
    * 3) there is no running process, priority_queue is not empty -> run the top node of the priority_queue
    * 4) there is a running process, priority_queue is not empty -> preempt
    */
    // to save process PIDs
    pid_t PIDS[MAX_NUM_PROCS];

    // local variables
    heap_node *running_node = NULL;
    min_heap priority_queue;
    priority_queue.size = 0;
    int executed_processes = 0;
    // keep running
    while (true)
    {
        // add new ready processes to the priority queue
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i]->state != READY) // check for the ready processes only
                continue;

            //creating a new heap node and setting its key to the process priority
            heap_node *node = (heap_node *)malloc(sizeof(heap_node));
            node->process = arrivalQ.processes[i];
            node->key = node->process->priority;
            node->process->state = RUNNING;
            min_heap_insert(&priority_queue, node);
            // to be used to calculate CPU utilization
            arrivalQ.processes[i]->remaining_time = arrivalQ.processes[i]->runningTime;
            arrivalQ.processes[i]->wait_time = 0;
            printf("Schduler : I have added process %d, with priority %d to priority queue\n", arrivalQ.processes[i]->id, arrivalQ.processes[i]->priority);
        }
        int currentTime = getClk();
        // there is no running process, priority_queue is not empty (case 3)
        if (running_node == NULL && priority_queue.size > 0)
        {
            // extract the node with the highest priority
            running_node = min_heap_extract(&priority_queue);

            // to be used in the log file
            if (running_node->process->remaining_time == running_node->process->runningTime)
                running_node->process->wait_time += currentTime - running_node->process->arrivalTime;
            else
                running_node->process->wait_time += currentTime - running_node->process->start_time;
            // set the start time to the currentTime
            running_node->process->start_time = currentTime;

            int j = running_node->process->id - 1; // to start from index 0

            // print to the log file
            char *status;
            if (running_node->process->state == SUSPENDED)
                status = "resumed";
            else
                status = "started";
            fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                    getClk(),                              //At time
                    running_node->process->id, status,     //process status
                    running_node->process->arrivalTime,           //arrival
                    running_node->process->runningTime,           //total
                    running_node->process->remaining_time, //remain
                    running_node->process->wait_time       //wait
            );
            // if the process is not suspended, run it
            if (running_node->process->state != SUSPENDED)
                PIDS[j] = fork_process(running_node->process->runningTime, running_node->process->id);
            else // send SIGCONT
                kill(PIDS[j], SIGCONT);
            printf("Schduler: process %d has started \n", running_node->process->id);
        }
        // there is a running process, priority_queue is not empty  (case 4)
        // if two processes have the same priority, run the one with the least remaining time
        else if (running_node != NULL && priority_queue.size > 0 && (priority_queue.heap[0]->process->priority < running_node->process->priority || priority_queue.heap[0]->process->priority == running_node->process->priority && priority_queue.heap[0]->process->remaining_time < running_node->process->remaining_time))
        {

            //preempt
            printf("Schduler: Preemption, switch process %d with process %d\n", running_node->process->id, priority_queue.heap[0]->process->id);
            running_node->process->state = SUSPENDED;
            running_node->process->priority--; // decremnt the process priority
            running_node->process->remaining_time = running_node->process->runningTime - (currentTime - running_node->process->start_time);
            running_node->key = running_node->process->priority; // Hence, the key ia also decremnted
            min_heap_insert(&priority_queue, running_node);
            // suspend the currentProcess
            kill(PIDS[running_node->process->id - 1], SIGSTOP);

            // print to the log file
            fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                    getClk(),                              //At time
                    running_node->process->id, "stopped",  //process stopped
                    running_node->process->arrivalTime,           //arrival
                    running_node->process->runningTime,           //total
                    running_node->process->remaining_time, //remain
                    running_node->process->wait_time       //wait
            );

            heap_node *new_running_node;
            new_running_node = min_heap_extract(&priority_queue);
            new_running_node->process->start_time = currentTime;
            char *status;
            if (new_running_node->process->state == SUSPENDED)
                status = "resumed";
            else
                status = "started";
            new_running_node->process->state = RUNNING;

            fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                    getClk(),                              //At time
                    new_running_node->process->id, status, //process status
                    new_running_node->process->arrivalTime,       //arrival
                    new_running_node->process->runningTime,       //total
                    new_running_node->process->runningTime,       //remain
                    new_running_node->process->wait_time   //wait
            );

            if (new_running_node->process->state != SUSPENDED)
                PIDS[new_running_node->process->id - 1] = fork_process(new_running_node->process->runningTime, new_running_node->process->id);
            else
                kill(PIDS[new_running_node->process->id - 1], SIGCONT);
            running_node = new_running_node;
        }
        // check if the running process has finished, print its info to the log file
        if (running_node != NULL && running_node->process->start_time + running_node->process->remaining_time == currentTime)
        {
            running_node->process->remaining_time = 0;
            int TA = getClk() - running_node->process->arrivalTime;
            float WTA = ((float)TA) / running_node->process->runningTime;

            fprintf(LogFile, SCHEDULER_LOG_FINISH_LINE_FORMAT,
                    getClk(),                              //At time
                    running_node->process->id, "finished", //process finished
                    running_node->process->arrivalTime,           //arrival
                    running_node->process->runningTime,           //total
                    running_node->process->remaining_time, //remain
                    running_node->process->wait_time,      //wait
                    TA,                                    //TA
                    WTA                                    //WTA
            );
            total_wait += running_node->process->wait_time;
            total_WTA += WTA;
            running_node->process->state = FINISHED;
            running_node = NULL;
            executed_processes++;
            process_completed = true;
        }

        // if all processes have finished, stop execution
        if (executed_processes == max_num_processes)
        {
            kill(getppid(), SIGINT);
        }
    }
}

void shortest_remaining_time_next(void)
{
    pid_t PIDS[MAX_NUM_PROCS];

    heap_node *running_node = NULL;
    min_heap priority_queue;
    priority_queue.size = 0;
    int executed_processes = 0;

    while (true)
    {
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i]->state != READY) //no ready process
            {
                continue;
            }

            //creating a new heap node and setting its priority to running time (shortest running time first)
            heap_node *node = (heap_node *)malloc(sizeof(heap_node));
            arrivalQ.processes[i]->remaining_time = arrivalQ.processes[i]->runningTime;
            arrivalQ.processes[i]->wait_time = 0;
            node->process = arrivalQ.processes[i];
            node->key = node->process->remaining_time;
            node->process->state = RUNNING;
            min_heap_insert(&priority_queue, node);
        }

        if (running_node == NULL && priority_queue.size > 0)
        {
            running_node = min_heap_extract(&priority_queue);

            if (running_node->process->remaining_time == running_node->process->runningTime)
            {
                running_node->process->wait_time += getClk() - running_node->process->arrivalTime;
            }
            else
            {
                running_node->process->wait_time += getClk() - running_node->process->start_time;
            }
            running_node->process->start_time = getClk();

            int j = running_node->process->id - 1; //to start from index 0

            char *status;
            if (running_node->process->state == SUSPENDED)
            {
                status = "resumed";
            }
            else
            {
                status = "started";
            }
            fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                    getClk(),                              //At time
                    running_node->process->id, status,     //process status
                    running_node->process->arrivalTime,           //arrival
                    running_node->process->runningTime,           //total
                    running_node->process->remaining_time, //remain
                    running_node->process->wait_time       //wait
            );

            if (running_node->process->state != SUSPENDED)
            {
                PIDS[j] = fork_process(running_node->process->runningTime, running_node->process->id);
            }
            else
            {
                kill(PIDS[j], SIGCONT);
            }
        }
        else
        {
            if (running_node != NULL && running_node->process->start_time + running_node->process->remaining_time == getClk()) //check if the running process is finished
            {
                running_node->process->remaining_time = 0;
                int TA = getClk() - running_node->process->arrivalTime;
                float WTA = ((float)TA) / running_node->process->runningTime;

                fprintf(LogFile, SCHEDULER_LOG_FINISH_LINE_FORMAT,
                        getClk(),                              //At time
                        running_node->process->id, "finished", //process finished
                        running_node->process->arrivalTime,           //arrival
                        running_node->process->runningTime,           //total
                        running_node->process->remaining_time, //remain
                        running_node->process->wait_time,      //wait
                        TA,                                    //TA
                        WTA                                    //WTA
                );
                total_wait += running_node->process->wait_time;
                total_WTA += WTA;
                running_node->process->state = FINISHED;
                running_node = NULL;
                executed_processes++;
                process_completed = true;
            }
            if (running_node != NULL && priority_queue.size > 0 && priority_queue.heap[0]->process->runningTime < running_node->process->runningTime - (getClk() - running_node->process->start_time))
            {
                //preempt
                running_node->process->state = SUSPENDED;
                running_node->process->remaining_time = running_node->process->runningTime - (getClk() - running_node->process->start_time);
                running_node->key = running_node->process->remaining_time;
                min_heap_insert(&priority_queue, running_node);
                kill(PIDS[running_node->process->id - 1], SIGSTOP);

                fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                        getClk(),                              //At time
                        running_node->process->id, "stopped",  //process stopped
                        running_node->process->arrivalTime,           //arrival
                        running_node->process->runningTime,           //total
                        running_node->process->remaining_time, //remain
                        running_node->process->wait_time       //wait
                );

                heap_node *new_running_node;
                new_running_node = min_heap_extract(&priority_queue);
                new_running_node->process->start_time = getClk();
                char *status;
                if (new_running_node->process->state == SUSPENDED)
                {
                    status = "resumed";
                }
                else
                {
                    status = "started";
                }
                new_running_node->process->state = RUNNING;

                fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                        getClk(),                              //At time
                        new_running_node->process->id, status, //process status
                        new_running_node->process->arrivalTime,       //arrival
                        new_running_node->process->runningTime,       //total
                        new_running_node->process->runningTime,       //remain
                        new_running_node->process->wait_time   //wait
                );

                if (new_running_node->process->state != SUSPENDED)
                {
                    PIDS[new_running_node->process->id - 1] = fork_process(new_running_node->process->runningTime, new_running_node->process->id);
                }
                else
                {
                    kill(PIDS[new_running_node->process->id - 1], SIGCONT);
                }
                running_node = new_running_node;
            }
        }
        if (executed_processes == max_num_processes)
        {
            kill(getppid(), SIGINT);
        }
    }
}

void round_robin(void)
{
    pid_t PIDS[MAX_NUM_PROCS];
    int Waits[MAX_NUM_PROCS];
    int last_interrupt_timestamps[MAX_NUM_PROCS];
    int remaining_times[MAX_NUM_PROCS];

    bool all_processes_finished;
    while (1)
    {
        all_processes_finished = true;
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i]->id == -1)
                continue;

            all_processes_finished = false;
            *(process_interrupt_flags + arrivalQ.processes[i]->id) = false;
            process_completed = false;
            if (arrivalQ.processes[i]->state == READY)
            {
                Waits[i] = getClk() - arrivalQ.processes[i]->arrivalTime;
                remaining_times[i] = arrivalQ.processes[i]->runningTime;
                *(process_remaining_flags + arrivalQ.processes[i]->id) = remaining_times[i];
                fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                        getClk(),                            //At time
                        arrivalQ.processes[i]->id, "started", //process started
                        arrivalQ.processes[i]->arrivalTime,          //arrival
                        arrivalQ.processes[i]->runningTime,          //total
                        remaining_times[i],                  //remain
                        Waits[i]                             //wait
                );
                PIDS[i] = fork_process(arrivalQ.processes[i]->runningTime, arrivalQ.processes[i]->id);
            }
            else
            {
                Waits[i] += getClk() - last_interrupt_timestamps[i];
                fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                        getClk(),                                 //At time
                        arrivalQ.processes[i]->id, "resumedraise", //process resumed
                        arrivalQ.processes[i]->arrivalTime,               //arrival
                        arrivalQ.processes[i]->runningTime,               //total
                        remaining_times[i],                       //remain
                        Waits[i]                                  //wait
                );
                kill(PIDS[i], SIGCONT);
            }

            int old = remaining_times[i];
            while ((old - remaining_times[i] != RR_quanta) && !process_completed)
            {
                remaining_times[i] = *(process_remaining_flags + arrivalQ.processes[i]->id);
            }
            *(process_interrupt_flags + arrivalQ.processes[i]->id) = true;

            if (remaining_times[i] == 0)
            {
                printf("Scheduler : rem for process %d dropped to zero \n reaping flag\n", arrivalQ.processes[i]->id);
                //reap the proess_completed flag
                while (!process_completed)
                    ;
                printf("\nReaped process %d", arrivalQ.processes[i]->id);
                int TA = getClk() - arrivalQ.processes[i]->arrivalTime;
                float WTA = ((float)TA) / arrivalQ.processes[i]->runningTime;
                fprintf(LogFile, SCHEDULER_LOG_FINISH_LINE_FORMAT,
                        getClk(),                             //At time
                        arrivalQ.processes[i]->id, "finished", //process finished
                        arrivalQ.processes[i]->arrivalTime,           //arrival
                        arrivalQ.processes[i]->runningTime,           //total
                        0,                                    //remain
                        Waits[i],                             //wait
                        TA,                                   //TA
                        WTA                                   //WTA
                );
                arrivalQ.processes[i]->id = -1;
            }
            else
            {
                fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT,
                        getClk(),                            //At time
                        arrivalQ.processes[i]->id, "stopped", //process stopped
                        arrivalQ.processes[i]->arrivalTime,          //arrival
                        arrivalQ.processes[i]->runningTime,          //total
                        remaining_times[i],                  //remain
                        Waits[i]                             //wait
                );
                last_interrupt_timestamps[i] = getClk();
                arrivalQ.processes[i]->state = SUSPENDED;
            }
        }

        if (all_processes_finished && arrivalQ.num_processes == max_num_processes)
            kill(getppid(), SIGINT);
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
        recieve_process();

    signal(SIGUSR1, on_msgqfull_handler);
}
/**
 * @brief mark the process as complete 
 * @param signum, signal number as it is a signal handler
 */
void on_process_complete_awake(int signum)
{
    process_completed = true;
    //printf("Schduler: a process has finished");
    signal(SIGUSR2, on_process_complete_awake);
}

/**
 * @brief free resources by closing the log file
 * @param signum 
 */
void free_resources(int signum)
{
    fclose(LogFile);

    LogFile = fopen("scheduler.perf", "w");
    int total_runtimes = 0;

    for (int i = 0; i < arrivalQ.num_processes; i++)
        total_runtimes += arrivalQ.processes[i]->runningTime;

    fprintf(LogFile, "CPU utilization=%.2f%%\n", ((float)total_runtimes) / (getClk() - 1) * 100);
    fprintf(LogFile, "Avg WTA=%.2f\n", total_WTA / arrivalQ.num_processes);
    fprintf(LogFile, "Avg Waiting=%.2f\n", total_wait / ((float)arrivalQ.num_processes));
    fclose(LogFile);

    for (int i = 0; i < arrivalQ.num_processes; i++)
    {
        free(arrivalQ.processes[i]);
    }    
    shmctl(process_interrupt_shmid, IPC_RMID, NULL);
    shmctl(process_remaining_shmid, IPC_RMID, NULL);

    printf("Schduler: I managed to close the log file\n");
    exit(0);
}
/**
 * @brief recieve new processes from the message queue, as long as the PCB table is not full,
 * in case that the PCB table is full, it ignores the recieved process.
 */
void recieve_process()
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
        //printf("Scheduler: process was recieved successfully\n");
    }
    if (arrivalQ.num_processes < arrivalQ.capacity) // check if there is space for a new process
    {
        // process paramerters
        arrivalQ.processes[arrivalQ.num_processes] = (proc*)malloc(sizeof(proc));
        arrivalQ.processes[arrivalQ.num_processes]->id = reply.currentProcess.id;
        arrivalQ.processes[arrivalQ.num_processes]->arrivalTime = reply.currentProcess.arrivalTime;
        arrivalQ.processes[arrivalQ.num_processes]->runningTime = reply.currentProcess.runningTime;
        arrivalQ.processes[arrivalQ.num_processes]->priority = reply.currentProcess.priority;
        // set its state as ready
        arrivalQ.processes[arrivalQ.num_processes]->state = READY;
        // inc the number of processes in the arrival queue
        arrivalQ.num_processes++;
    }
    else
        printf("Scheduler: Error recieving a process... \nPCB array is full... Process Ignored");
}
/**
 * @brief his function is responsible for running a process by froking it, it creates a new 
 * child then, use execv to start it as a new process .
 * @param runtime  indicates the remainging time for this process to finish execution
 * @param id       The index in the arrivalQ where that process is located
 * @return pid_t, Return -1 for errors.
 */
pid_t fork_process(int runtime, int id)
{
    pid_t process_pid = fork();

    if (process_pid == 0) // if forking went sucessful
    {
        char runtime_as_string[12], id_as_string[12];
        sprintf(runtime_as_string, "%d", runtime);
        sprintf(id_as_string, "%d", id);

        char *args[] = {runtime_as_string, id_as_string, NULL};
        while (execv("./process.out", args) == -1)
        {
            printf("Scheduler clone : Failed to create a new process... trying again");
        }
    }
    else
        return process_pid;
}

//LEAVE THIS*************************************************
/*
 if (running_process == NULL && priority_queue.size > 0)      //there is no running process currently and the queue has nodes
        {
            running_process = min_heap_extract(&priority_queue);
            running_process->process->start_time = getClk();
            running_process->process->state = RUNNING;
            *(process_interrupt_flags + running_process->process->id) = false;
            //fprintf(LogFile, "run (and remove from the heap) process#%d at time = %d\n\n", running_process->process->id, getClk());
            fork_process(running_process->process->runningTime, running_process->process->id);
        }
        else
        {
            //fprintf(LogFile, "running process#%d, expected finish time = %d\n", running_process->id, running_process->start_time + running_process->runningTime);
            if (running_process != NULL && running_process->process->start_time + running_process->process->runningTime == getClk()) //check if the running process is finished
            {   
                if (running_process->process->state != FINISHED)
                {
                    fprintf(LogFile, "process #%d finished at time = %d\n\n", running_process->process->id, getClk());
                }
                running_process->process->state = FINISHED;
                running_process->process->finish_time = getClk();
                running_process = NULL;
            }
            // fprintf(LogFile, "running process runtime = %d\n", running_process->runningTime);
            // fprintf(LogFile, "minimum runtime in the heap = %d, id = %d\n", priority_queue.heap[0]->process->runningTime, priority_queue.heap[0]->process->id);
            // fprintf(LogFile, "running = %d\n", running);
            if (running_process != NULL && priority_queue.size > 0 && priority_queue.heap[0]->process->runningTime < running_process->process->runningTime - (getClk() - running_process->process->start_time))
            {
                //preempt
                heap_node* new_running_node;
                new_running_node = min_heap_extract(&priority_queue);
                new_running_node->process->start_time = getClk();
                new_running_node->process->state = RUNNING;

                fprintf(LogFile, "run process#%d (extract from the min heap) ", new_running_node->process->id);

                heap_node* temp_node;
                running_process->process->state = READY;
                running_process->process->runningTime = running_process->process->runningTime - (getClk() - running_process->process->start_time);
                temp_node = running_process;
                temp_node->key = temp_node->process->runningTime;

                running_process = new_running_node;
                fprintf(LogFile, "--- add process#%d to min heap at time = %d (preemption)\n\n", temp_node->process->id, getClk());
            }
        }
*/