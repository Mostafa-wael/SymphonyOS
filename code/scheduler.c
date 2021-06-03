#include "headers.h"
#include "processTable.h"
#include <string.h>
struct msgbuff
{
    long mtype;
    comingProcess currentProcess;
};

#define MAX_NUM_PROCS 100 

#define FIRST_COME_FIRST_SERVE "FCFS"
#define SHORTEST_JOB_FIRST     "SJF"
#define PREEMPTIVE_HIGHEST_PRIORITY_FIRST "PHPF"
#define SHORTEST_REMAINING_TIME_NEXT      "SRTN"
#define ROUND_ROBIN "RR"

int  MsgQID ;
int* MsgQIDsz ;

enum proc_state {READY, SUSPENDED};
struct proc {
    int id ;
    int arrt ;
    int runt ;
    int priorty;
    enum proc_state state;
};
struct {
    struct proc* processes ;
    int num_processes  ;
    int capacity;
} arrivalQ ;

void recieve_proc(){
    struct msgbuff reply ;
    msgrcv(MsgQID,&reply,sizeof(struct msgbuff) - sizeof(long),0,!IPC_NOWAIT);
    
    if(arrivalQ.num_processes < arrivalQ.capacity){
        arrivalQ.processes[arrivalQ.num_processes].id      = reply.currentProcess.id ;
        arrivalQ.processes[arrivalQ.num_processes].arrt    = reply.currentProcess.arrivaltime ;
        arrivalQ.processes[arrivalQ.num_processes].runt    = reply.currentProcess.runningtime ;
        arrivalQ.processes[arrivalQ.num_processes].priorty = reply.currentProcess.priority ;
        arrivalQ.processes[arrivalQ.num_processes].state   = READY ;
        arrivalQ.num_processes++ ;
    } else printf("Scheduler : Error recieving a process... \nPCB array is full... Process Ignored");
}
pid_t fork_process(int);

void first_come_first_serve(void);
void round_robin(void);



void on_msgqfull_handler(int);

bool process_completed ;
void on_process_complete_awake(int);

void free_resources(int);
FILE* LogFile ;

int RR_quanta ;
int main(int argc, char *argv[])
{   
    MsgQID        = msgget(ftok("keyfile", 'S'), 0666 | IPC_CREAT); 
    int MsqQIDszSHMID = shmget(ftok("keyfile", 'M'),4,0666|IPC_CREAT);
    MsgQIDsz      = (int*)shmat(MsqQIDszSHMID,NULL,0);
    
    arrivalQ.processes = (struct proc*)malloc(MAX_NUM_PROCS*sizeof(struct proc));
    arrivalQ.capacity = MAX_NUM_PROCS ;
    arrivalQ.num_processes = 0;

    signal(SIGUSR1,on_msgqfull_handler);
    signal(SIGUSR2,on_process_complete_awake);
    signal(SIGINT,free_resources);
    initClk();

    void (*algos_ptrs[5])(void);
    algos_ptrs[0] = first_come_first_serve ;
    algos_ptrs[1] = NULL ;
    algos_ptrs[2] = NULL ;
    algos_ptrs[3] = NULL ;
    algos_ptrs[4] = round_robin ;

    int algo_idx = -1 ;
    printf("Schduler : called on %s\n",argv[0]);
    if( (LogFile = fopen("scheduler.log","w")) 
    == NULL)  printf("Scheduler: Failed to open log file");
    
         if(strcmp(argv[0], FIRST_COME_FIRST_SERVE)            == 0) algo_idx = 0;
    else if(strcmp(argv[0], SHORTEST_JOB_FIRST)                == 0) algo_idx = 1;
    else if(strcmp(argv[0], PREEMPTIVE_HIGHEST_PRIORITY_FIRST) == 0) algo_idx = 2;
    else if(strcmp(argv[0], SHORTEST_REMAINING_TIME_NEXT)      == 0) algo_idx = 3;
    else{
        RR_quanta = atoi(argv[1]);
        algo_idx = 4;
    }

    if(algos_ptrs[algo_idx] != NULL) algos_ptrs[algo_idx]();
    else{
        perror("scheduler : Called with an algorithm that wasn't implemented yet");
        exit(-1);
    } 
}

void first_come_first_serve(void){
    while(1){
     
        for(int i = 0; i < arrivalQ.num_processes; i++){
            if(arrivalQ.processes[i].id == -1) continue ;
            
            fprintf(LogFile,"at %d Process with id = %d commencing now \n", getClk(),arrivalQ.processes[i].id);
            fork_process(arrivalQ.processes[i].runt);


            //the sleep will not complete off course 
            //but we are putting it in a while loop because it might be interrupted for a reason other
            //than the process finished
            process_completed = false ;
            while(!process_completed) sleep(__INT_MAX__);
            fprintf(LogFile,"at %d Process %d ran to completion\n",getClk(),arrivalQ.processes[i].id);
         
            arrivalQ.processes[i].id = -1;
        }
    }       
}

void round_robin(void){
    pid_t PIDS[MAX_NUM_PROCS];
    while (1)
    {
        for(int i = 0; i<arrivalQ.num_processes; i++){
            if(arrivalQ.processes[i].id == -1) continue ;
            
            process_completed = false ;
            if(arrivalQ.processes[i].state == READY){
                fprintf(LogFile,"at %d Process with id = %d commencing now \n", getClk(),arrivalQ.processes[i].id);

                PIDS[i] = fork_process(arrivalQ.processes[i].runt);
            }
            else{
                fprintf(LogFile,"at %d suspended Process with id = %d resuming now \n", getClk(),arrivalQ.processes[i].id);
                kill(PIDS[i],SIGUSR2);
            }

            int curr_time = getClk();
            while(getClk() - curr_time < RR_quanta){ 
                if(process_completed) {
                    fprintf(LogFile,"\n#process %d signalled completion before quanta end#\n--\n",arrivalQ.processes[i].id);
                    break;
                }
            }

            arrivalQ.processes[i].runt -= RR_quanta ;
            if(arrivalQ.processes[i].runt <= 0){
                fprintf(LogFile,"at %d Process with id = %d ran to completion \n", getClk(),arrivalQ.processes[i].id);
                while(!process_completed);
                arrivalQ.processes[i].id = -1;
            }
            else{
                fprintf(LogFile,"at %d Process with id = %d is suspended, Remaning time %d\n", getClk(),arrivalQ.processes[i].id,arrivalQ.processes[i].runt);

                kill(PIDS[i],SIGUSR1);
                arrivalQ.processes[i].state = SUSPENDED ;
            }

        }
    }
}
    


void on_msgqfull_handler(int signum){
    for(; (*MsgQIDsz) > 0; (*MsgQIDsz)--)  recieve_proc();

    signal(SIGUSR1,on_msgqfull_handler);
}

void on_process_complete_awake(int signum){
    process_completed = true ;
    signal(SIGUSR2,on_process_complete_awake);
}

void free_resources(int signum){
    fclose(LogFile);
        
    exit(0);
}

pid_t fork_process(int runtime){
    pid_t process_pid = fork();

    if(process_pid == 0){
        char runtime_as_string[12];
        sprintf(runtime_as_string,"%d",runtime);

        char* args[] = {runtime_as_string,NULL};
        while(execv("./process.out",args) == -1){
            printf("Scheduler clone : Failed to create a new process... trying again");
        }
    }
    else return process_pid ; 
}