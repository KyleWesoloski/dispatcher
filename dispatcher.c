//Wesoloski.Kyle.

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "signal.h" 
#include "sys/wait.h"
#define MAX_COMMANDS 1001


struct Queue
{
	int head; 							//first command in each queue
	int tail; 							//delete this
	int commands;						//number of commands in each queue
	int processRunning[MAX_COMMANDS];; 	//flag if a queue is running a process currently 
	char *q[MAX_COMMANDS];				//arrival,priority,and time remaining of each process; 
	char* arrive[MAX_COMMANDS];			//arrival time of each process
	char* prior[MAX_COMMANDS]; 			//priority of each process
	int time[MAX_COMMANDS]; 			//time remaining of each process
	pid_t pid[MAX_COMMANDS]; 			//array to store current PIDs
	
};

struct Queue dispatch = {0,-1,0}; 		//dispatch queue
struct Queue systemQ = {0,-1,0};		//system queue
struct Queue priorOne = {0,-1,0};		//priority queues 
struct Queue priorTwo = {0,-1,0};		//
struct Queue priorThree = {0,-1,0};		//

void terminateProcess(struct Queue *X);								//kill process
void suspendProcess(struct Queue *X); 								//pause process
void insert(struct Queue *X, char* p[3]);							//insert process into queue
void reQueueProcess(struct Queue *start, struct Queue *dest);		//insert process from one queue to another
void startProcess(struct Queue *X);									//create new process
void restartProcess(struct Queue *X);								//run process previously started

void insert(struct Queue *X, char* p[3]){														//add process to queue
	X->commands++; 
	X->arrive[(X->commands+X->head - 1)%(MAX_COMMANDS)] = (char*)malloc(sizeof(char)*4);		//store arrival, priority, time remaining, and
	X->prior[(X->commands+X->head - 1)%(MAX_COMMANDS)] = (char*)malloc(sizeof(char)*4);			//processRunning into appropriate queue
	X->arrive[(X->commands+X->head -1)%(MAX_COMMANDS)] = p[0]; 									//
	X->prior[(X->commands+X->head -1)%MAX_COMMANDS] = p[1];										//
	X->time[(X->commands+X->head -1)%MAX_COMMANDS] = atoi(p[2]);								//
	X->processRunning[(X->commands+X->head -1)%MAX_COMMANDS] = 0; 								//
}

void reQueueProcess(struct Queue *start, struct Queue *dest){
	char  t[2]; 
	char* priority = "3"; 
	sprintf(t, "%d", start->time[(start->head)%MAX_COMMANDS]); //store time remaining into string 
	if (strcmp(start->prior[start->head%MAX_COMMANDS],"1") == 0) priority = "2"; 
	char * args[3] = {start->arrive[start->head%MAX_COMMANDS],priority,t}; 	//create argument for insert 
	insert(dest, args);																						//insert process into new queue
	dest->pid[(dest->head+dest->commands-1)%MAX_COMMANDS] = start->pid[(start->head)%MAX_COMMANDS]; 		//store pid from process into new queue position
	dest->processRunning[(dest->head+dest->commands-1)%MAX_COMMANDS] = 1;									//set process running equal to true
}

void startProcess(struct Queue *X){
	X->processRunning[(X->head)%MAX_COMMANDS] = 1; 															//set process running equal to true
	pid_t pid;   	
	pid = fork(); 																							//create new process 
	X->pid[(X->head)%MAX_COMMANDS] = pid; 																	//store pid of process, not necessary for system Queue since it is FCFS
	char * args[3]; 																						//create args for execvp 
	char  str[2]; 
	sprintf(str, "%d", X->time[(X->head)%MAX_COMMANDS]);													//store time remaining into string 
	args[0] = "./process"; 																					//command name
	args[1] = str ;																							//time remaining
	args[2] = '\0'; 																						//null terminating char
	if (pid < 0) fprintf(stderr, "Fork Failed"); 															//fork fails 
	else if (pid == 0) execvp("./process", args);															//run child process
	else {																									//parent process
		X->time[(X->head)%MAX_COMMANDS]--;																	//decrease time remaining of command
		sleep(1);																							//run for one time quantum 
		if(strcmp(X->prior[X->head%MAX_COMMANDS],"0") != 0){												//if a system process, continue 
			if(X->time[(X->head)%MAX_COMMANDS] == 0) terminateProcess(X); 										//if process is over, kill it
			else{													
			if (strcmp(X->prior[X->head%MAX_COMMANDS],"1") == 0) reQueueProcess(X,&priorTwo); 				//move priority process 1 to 2
			else if (strcmp(X->prior[X->head%MAX_COMMANDS],"2") == 0)reQueueProcess(X,&priorThree);			//move 2 to 3
			else reQueueProcess(X,&priorThree); 															//move 3 to end of 3
			suspendProcess(X); 																				//suspend the process 
			}
			waitpid(X->pid[(X->head-1)%MAX_COMMANDS],NULL,WUNTRACED);										//wait for process signal 
		}
	}
}

void terminateProcess(struct Queue *X){
	kill(X->pid[(X->head)%MAX_COMMANDS], SIGINT); 				//end process
	X->processRunning[(X->head)%MAX_COMMANDS] = 0;  			//set processRunnning to false
	X->head++; 													//go to next command in queue
	X->commands--;  											//decrement number of commands in queue
}

void suspendProcess(struct Queue *X){
	kill(X->pid[X->head%MAX_COMMANDS], SIGTSTP); 				//pause process
	X->processRunning[(X->head)%MAX_COMMANDS] = 0; 				//set processRunning to false
	X->head++; 								  					//got to next command in queue
	X->commands--; 												//decrement number of commands in queue
}

void restartProcess(struct Queue *X){											//restart process
		X->time[(X->head)%MAX_COMMANDS]--;										//decrement time remaining on process 
		kill(X->pid[(X->head)%MAX_COMMANDS], SIGCONT);							//send signal to continue process 
		sleep(1);																//stop for one time quantum
		
		if(X->time[(X->head)%MAX_COMMANDS] == 0) terminateProcess(X); 				//if process is done kill the process 
		else if (strcmp(X->prior[X->head%MAX_COMMANDS],"2") == 0) {				//requeue process from priority 2 to 3
			reQueueProcess(&priorTwo, &priorThree); 
			suspendProcess(X); 
		}
		else{
			struct Queue temp = priorThree; 										//requeue process from priority 3 to end of 3
			reQueueProcess(&temp, &priorThree); 
			suspendProcess(X);
		}
		waitpid(X->pid[(X->head-1)%MAX_COMMANDS],NULL,WUNTRACED); 					//wait for signal from process to continue 
}


int main(int argc, char* argv[])
{ 
	FILE *fp = fopen(argv[1], "r"); 
	dispatch.q[dispatch.commands] = (char*)malloc(sizeof(char)*20); 

	while(fgets(dispatch.q[dispatch.commands], 20, fp) != NULL)			//read file into dispatch queue
	{
		dispatch.tail = dispatch.commands; 
		dispatch.commands++; 
		dispatch.q[dispatch.commands] = (char*)malloc(sizeof(char)*30);
	}
	dispatch.commands--; 											
	int time = 0;														//global clock
	int should_run = 1; 												//keep running until all processes are finished
	int current_process = 0; 											
	int j;
	while(should_run == 1){	 
		for(j = current_process; j < dispatch.commands; j++){					//move commands from dispatcher queue if there are still commands
			char *process[3];														//holds values of each process
			char* temp = (char*)malloc(sizeof(char)*20);							//create copy of dispatch queue element
			strcpy(temp,dispatch.q[current_process]); 
			char *token = strtok(temp, ", ");
			int x = 0; 
			while( token != NULL ) 
			{	
				process[x] = (char*)malloc(sizeof(char)*3);							//store arrival, time, and priority into process[x]
				process[x] = token; 
				token = strtok(NULL, ", ");
				x++; 
			}
		
			if(atoi(process[0]) > time) {time++; break;} 							//no commands arrived at this time, increment time
			else if(atoi(process[0]) == time){										//command has arrived, insert into queue
				if(atoi(process[1]) == 0) insert(&systemQ, process);  
				else if(atoi(process[1]) == 1)insert(&priorOne, process); 
				else if(atoi(process[1]) == 2)insert(&priorTwo, process); 
				else insert(&priorThree, process); 
				current_process++; 													//move to next process in dispatcher queue
			}
									
		}
		
		if (systemQ.commands > 0){  																	//if command in system queue
			if(!systemQ.processRunning[systemQ.head%MAX_COMMANDS]){ 									//create new process if one doesn't exist 
				startProcess(&systemQ); 																//initiate new process
			} 
			else {systemQ.time[systemQ.head]--; sleep(1);}												//decrement process time by one 
			if(systemQ.time[systemQ.head] == 0) {														//process is finished					
				terminateProcess(&systemQ);																	//wait for child to be killed
				waitpid(systemQ.pid[(systemQ.head-1)%MAX_COMMANDS],NULL,WUNTRACED);
			}	
		}
		
		else if(priorOne.commands > 0) {																//if command in priority one queue
			if(!priorOne.processRunning[(priorOne.head)%MAX_COMMANDS]) startProcess(&priorOne); 		//create new process 
		}
		else if(priorTwo.commands > 0) {  																//if command in priority two queue
			if(!priorTwo.processRunning[(priorTwo.head)%MAX_COMMANDS]) startProcess(&priorTwo);		//create new process if not running 
			else restartProcess(&priorTwo);  															//restart process if already started 
		}								
		else if(priorThree.commands> 0) { 																//if command in priority two queue
			if(!priorThree.processRunning[(priorThree.head)%MAX_COMMANDS]) startProcess(&priorThree);	//create new process if not running 
			else restartProcess(&priorThree);  															//restart process if already started 
		}
		else should_run = 0; 
	}
	fclose(fp); 
	return 0; 
}

