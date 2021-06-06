#include "headers.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//************************************ #defines ***********************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
//Remember : writing two strings next to each other is equivalent to concatenating them 
//for example : printf("hello" "world") prints hello world to the console 

//At time 1 process 1 started arr 1 total 6 remain 6 wait 0

#define AT_TIME__ "At\ttime\t%d\t"
#define PROC__    "process\t%d\t%s\t"
#define ARR__     "arr\t%d\t"
#define TOTAL__   "total\t%d\t"
#define REMAIN__  "remain\t%d\t"
#define WAIT__    "wait\t%d"
#define SCHEDULER_LOG_NON_FINISH_LINE_FORMAT AT_TIME__ PROC__ ARR__ TOTAL__ REMAIN__ WAIT__ "\n"
#define SCHEDULER_LOG_FINISH_LINE_FORMAT     AT_TIME__ PROC__ ARR__ TOTAL__ REMAIN__ WAIT__ "\tTA\t%d" "\tWTA\t%.2f\n"
///////////////////////////////////////////////////////////////////////////////////////////////////
//************************************ globals***********************************//
/////////////////////////////////////////////////////////////////////////////////////////////////
int MsgQID;
int *MsgQIDsz;

bool process_completed;
FILE *LogFile;

int process_interrupt_semaphores ;
int RR_quanta;

int total_wait = 0;
float total_WTA = 0.0;

int max_num_processes ;
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
void shortest_remaining_time_next(void);
void shortest_job_first(void);
//
void on_msgqfull_handler(int);
void on_process_complete_awake(int);
pid_t fork_process(int,int);
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
    process_interrupt_semaphores = semget(ftok("keyfile",SEMSGKEY),100,0666|IPC_CREAT);

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
    algos_ptrs[1] = shortest_job_first;
    algos_ptrs[2] = PreemptiveHighestPriorityFirst;
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
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i].id == -1)
                continue;

            setSemaphoreValue(process_interrupt_semaphores,arrivalQ.processes[i].id);
            int wait = getClk() - arrivalQ.processes[i].arrt ;
            fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT, 
                            getClk(),                              //At time 
                            arrivalQ.processes[i].id,"started",    //process started
                            arrivalQ.processes[i].arrt,            //arrival 
                            arrivalQ.processes[i].runt,            //total
                            arrivalQ.processes[i].runt,            //remain
                            wait                                   //wait
                            ); 
            fork_process(arrivalQ.processes[i].runt,arrivalQ.processes[i].id);

            //the sleep will not complete off course
            //but we are putting it in a while loop because it might be interrupted for a reason other
            //than the process finished
            process_completed = false;
            while (!process_completed)
                sleep(__INT_MAX__);

            int TA = getClk()-arrivalQ.processes[i].arrt;
            float WTA = ((float)TA)/((float)arrivalQ.processes[i].runt) ;
            fprintf(LogFile, SCHEDULER_LOG_FINISH_LINE_FORMAT,
                            getClk(),                                        //At time 
                            arrivalQ.processes[i].id,"finished",             //process started
                            arrivalQ.processes[i].arrt,                      //arrival
                            arrivalQ.processes[i].runt,                      //total
                            0,                                               //remain
                            wait,                                            //wait
                            TA ,                                             //TA
                            WTA                                              //WTA
                            ); 


            arrivalQ.processes[i].id = -1;

            total_wait += wait ;
            total_WTA  += WTA  ; 
        }
        if(arrivalQ.num_processes == max_num_processes) kill(getppid(),SIGINT);
    }
}
void round_robin(void)
{
    pid_t PIDS[MAX_NUM_PROCS];
    int   Waits[MAX_NUM_PROCS];
    int   last_interrupt_timestamps[MAX_NUM_PROCS];
    int   remaining_times[MAX_NUM_PROCS];

    bool all_processes_finished ;
    while (1)
    {
        all_processes_finished = true ;
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i].id == -1)
                continue;

            all_processes_finished = false ;
            process_completed = false;
            setSemaphoreValue(process_interrupt_semaphores,arrivalQ.processes[i].id);

            if (arrivalQ.processes[i].state == READY)
            {
                Waits[i] = getClk() - arrivalQ.processes[i].arrt; 
                remaining_times[i] =  arrivalQ.processes[i].runt;

                fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT, 
                            getClk(),                              //At time 
                            arrivalQ.processes[i].id,"started",    //process started
                            arrivalQ.processes[i].arrt,            //arrival 
                            arrivalQ.processes[i].runt,            //total
                            remaining_times[i],                    //remain
                            Waits[i]                               //wait
                            ); 
                PIDS[i] = fork_process(arrivalQ.processes[i].runt,arrivalQ.processes[i].id);
            }
            else
            {
                Waits[i] += getClk() - last_interrupt_timestamps[i];
                fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT, 
                            getClk(),                              //At time 
                            arrivalQ.processes[i].id,"resumed",    //process resumed
                            arrivalQ.processes[i].arrt,            //arrival 
                            arrivalQ.processes[i].runt,            //total
                            remaining_times[i],                    //remain
                            Waits[i]                               //wait
                            ); 
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

            remaining_times[i] -= getClk()-curr_time;
            clearSemaphoreValue(process_interrupt_semaphores,arrivalQ.processes[i].id);

            if (remaining_times[i] <= 0)
            {
                while (!process_completed)
                    ;

                int TA ;
                fprintf(LogFile, SCHEDULER_LOG_FINISH_LINE_FORMAT, 
                            getClk(),                                        //At time 
                            arrivalQ.processes[i].id,"finished",             //process finished
                            arrivalQ.processes[i].arrt,                      //arrival 
                            arrivalQ.processes[i].runt,                      //total
                            remaining_times[i],                              //remain
                            Waits[i],                                        //wait
                            (TA = getClk()-arrivalQ.processes[i].arrt),      //TA
                            ((float)TA)/arrivalQ.processes[i].runt           //WTA
                            ); 
                arrivalQ.processes[i].id = -1;
            }
            else
            {
                fprintf(LogFile, SCHEDULER_LOG_NON_FINISH_LINE_FORMAT, 
                            getClk(),                              //At time 
                            arrivalQ.processes[i].id,"stopped",    //process stopped
                            arrivalQ.processes[i].arrt,            //arrival 
                            arrivalQ.processes[i].runt,            //total
                            remaining_times[i],                    //remain
                            Waits[i]                               //wait
                            ); 

                last_interrupt_timestamps[i] = getClk();
                arrivalQ.processes[i].state = SUSPENDED;
            }
        }
        if(all_processes_finished && arrivalQ.num_processes == max_num_processes) kill(getppid(),SIGINT);
    }
}
void shortest_job_first(void)
{
    min_heap priority_queue;

    while(true)
    {
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i].state != READY)
            {
                continue;
            }    

            //creating a new heap node and setting its priority to running time (shortest running time first)
            heap_node* node = (heap_node*)malloc(sizeof(heap_node));
            node->process = &arrivalQ.processes[i];
            node->key = node->process->runt;
            node->process->state = RUNNING;
            min_heap_insert(&priority_queue, node);
            fprintf(LogFile, "added to min heap process#%d at time %d\n\n", node->process->id, getClk());
        }

        if (priority_queue.size > 0)   //there are processes to be scheduled
        {
            proc* process = min_heap_extract(&priority_queue)->process;
            fork_process(process->runt, process->id);

            process_completed = false;
            while (!process_completed)
            {
                sleep(__INT_MAX__);
            }
            process->state = FINISHED;
            process->finish_time = getClk();
            fprintf(LogFile, "process #%d finished at time %d\n\n", process->id, getClk());
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
void shortest_remaining_time_next(void)
{
    heap_node* running_process = NULL;   //keep the running process
    min_heap priority_queue;
    priority_queue.size = 0;
    int prev_clock = 0;

    while(true)
    {
        for (int i = 0; i < arrivalQ.num_processes; i++)
        {
            if (arrivalQ.processes[i].state != READY)   //no ready process
            {
                continue;
            }    
            //creating a new heap node and setting its priority to running time (shortest running time first)
            heap_node* node = (heap_node*)malloc(sizeof(heap_node));       
            node->process = &arrivalQ.processes[i];         
            node->key = node->process->runt;         
            node->process->state = RUNNING;      
            min_heap_insert(&priority_queue, node);
            fprintf(LogFile, "added to min heap: process#%d at time = %d\n\n", node->process->id, getClk());
            
            if (prev_clock + 1 == getClk())
            {
                prev_clock = getClk();
                if (running_process == NULL && priority_queue.size > 0)      //there is no running process currently and the queue has nodes
                {
            running_process = min_heap_extract(&priority_queue);
            running_process->process->start_time = getClk();
            running_process->process->state = RUNNING;
            fprintf(LogFile, "run (and remove from the heap) process#%d at time = %d\n\n", running_process->process->id, getClk());
            fork_process(running_process->process->runt, running_process->process->id);
        }
        else
        {
            //fprintf(LogFile, "running process#%d, expected finish time = %d\n", running_process->id, running_process->start_time + running_process->runt);
            if (running_process != NULL && running_process->process->start_time + running_process->process->runt == getClk()) //check if the running process is finished
            {   
                if (running_process->process->state != FINISHED)
                {
                    fprintf(LogFile, "process #%d finished at time = %d\n\n", running_process->process->id, getClk());
                }
                running_process->process->state = FINISHED;
                running_process->process->finish_time = getClk();
                running_process = NULL;
            }
            // fprintf(LogFile, "running process runtime = %d\n", running_process->runt);
            // fprintf(LogFile, "minimum runtime in the heap = %d, id = %d\n", priority_queue.heap[0]->process->runt, priority_queue.heap[0]->process->id);
            // fprintf(LogFile, "running = %d\n", running);
            if (running_process != NULL && priority_queue.size > 0 && priority_queue.heap[0]->process->runt < running_process->process->runt - (getClk() - running_process->process->start_time))
            {
                //preempt
                heap_node* new_running_node;
                new_running_node = min_heap_extract(&priority_queue);
                new_running_node->process->start_time = getClk();
                new_running_node->process->state = RUNNING;

                fprintf(LogFile, "run process#%d (extract from the min heap) ", new_running_node->process->id);

                heap_node* temp_node;
                running_process->process->state = READY;
                running_process->process->runt = running_process->process->runt - (getClk() - running_process->process->start_time);
                temp_node = running_process;
                temp_node->key = temp_node->process->runt;

                running_process = new_running_node;
                fprintf(LogFile, "--- add process#%d to min heap at time = %d (preemption)\n\n", temp_node->process->id, getClk());
            }
        } 
            }
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

    LogFile = fopen("scheduler.perf","w");
    int total_runtimes = 0;
    for(int i = 0; i<arrivalQ.num_processes; i++) total_runtimes += arrivalQ.processes[i].runt ;

    fprintf(LogFile,"CPU utilization=%.2f%%\n",((float)total_runtimes)/(getClk()-1));
    fprintf(LogFile,"Avg WTA=%.2f\n",total_WTA/arrivalQ.num_processes);
    fprintf(LogFile,"Avg Waiting=%.2f\n",total_wait/((float)arrivalQ.num_processes));
    fclose(LogFile);

    semctl(process_interrupt_semaphores,-1/*Ignored*/,IPC_RMID);
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
        sprintf(id_as_string,"%d",id);

        char *args[] = {runtime_as_string,id_as_string, NULL};
        while (execv("./process.out", args) == -1)
        {
            printf("Scheduler clone : Failed to create a new process... trying again");
        }
    }
    else
        return process_pid;
}
