#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <stdlib.h>

#include "disastrOS.h"
#include "fixed_size_message_queue.h"

#define SEM_FULL 0
#define SEM_EMPTY 1
#define THREAD_SEM 2

typedef struct ThreadArgs{
  int type;
  int id;
  int sleep_time;
  int cycles;
  FixedSizeMessageQueue* queue;
}ThreadArgs;

// we need this to handle the sleep state
void sleeperFunction(void* args){
  printf("Hello, I am the sleeper, and I sleep %d\n",disastrOS_getpid());
  while(1) {
    getc(stdin);
    disastrOS_printStatus();
  }
}
void prodFunction(void* arg){
  printf("prod:::%d started\n",disastrOS_getpid());
	
	ThreadArgs* args = (ThreadArgs*)arg;

  int thread_sem = disastrOS_semOpen(THREAD_SEM,-1);
  if(thread_sem<0){
    printf("prod:::%d could not open sem:%d\n",disastrOS_getpid(),THREAD_SEM);
    disastrOS_exit(-1);  
  }

	int sem_empty = disastrOS_semOpen(SEM_EMPTY,-1);
  if(sem_empty<0){
    printf("prod:::%d could not open sem:%d\n",disastrOS_getpid(),SEM_EMPTY);
    disastrOS_exit(-1);  
  }

	int sem_full = disastrOS_semOpen(SEM_FULL,-1);
  if(sem_full<0){
    printf("prod:::%d could not open sem:%d\n",disastrOS_getpid(),SEM_FULL);
    disastrOS_exit(-1);  
  }

  for (int i=0; i<args->cycles; ++i) {
    char buf[1024];
    sprintf(buf, "msg from %d, cycle: %d", args->id,i);
    int length=strlen(buf);
    char* msg=(char*)malloc(length);
    strcpy(msg, buf);

    printf("INFO, PRODUCER  %d sending [%s] \n", args->id,msg);
    
    FixedSizeMessageQueue_pushBack(args->queue, msg, sem_empty, sem_full, thread_sem);
    disastrOS_sleep(args->sleep_time);
  }	

  printf("prod:::%d terminated\n",disastrOS_getpid());
  disastrOS_exit(disastrOS_getpid()+1);
}

void consFunction(void* arg){
  printf("cons:::%d started\n",disastrOS_getpid());
	
	ThreadArgs* args = (ThreadArgs*)arg;

  int thread_sem = disastrOS_semOpen(THREAD_SEM,-1);
  if(thread_sem<0){
    printf("cons:::%d could not open sem:%d\n",disastrOS_getpid(),THREAD_SEM);
    disastrOS_exit(-1);  
  }

	int sem_empty = disastrOS_semOpen(SEM_EMPTY,-1);
  if(sem_empty<0){
    printf("cons:::%d could not open sem:%d\n",disastrOS_getpid(),SEM_EMPTY);
    disastrOS_exit(-1);  
  }

	int sem_full = disastrOS_semOpen(SEM_FULL,-1);
  if(sem_full<0){
    printf("cons:::%d could not open sem:%d\n",disastrOS_getpid(),SEM_FULL);
    disastrOS_exit(-1);  
  }

  for (int i=0; i<args->cycles; ++i) {
    printf("INFO, CONSUMER  %d waiting\n", args->id);
    char* msg=FixedSizeMessageQueue_popFront(args->queue, sem_empty, sem_full, thread_sem);
    printf("INFO, CONSUMER  %d receiving [%s] \n", args->id,msg);
    disastrOS_sleep(args->sleep_time);
  }

  printf("cons:::%d terminated\n",disastrOS_getpid());
  disastrOS_exit(disastrOS_getpid()+1);
}

void initFunction(void* args) {
  disastrOS_printStatus();
  printf("hello, I am init and I just started\n");
  printf("DisastrOS spawning sleeper thread\n");
  disastrOS_spawn(sleeperFunction, 0);
  
  //Dichiaro variabili utili alla coda di messaggi e la creo
  int sem_empty, sem_full, thread_sem, size_max = 10;
  FixedSizeMessageQueue q;
  FixedSizeMessageQueue_init(&q, size_max, &sem_empty, &sem_full, &thread_sem);

	ThreadArgs producer_args_template = {0, 0, 1, 10, &q};
  ThreadArgs consumer_args_template = {1, 0, 2, 10, &q};	
	
	int alive_children=0;
  printf("DisastrOS spawning producers\n");
  disastrOS_spawn(&prodFunction, (void*)&producer_args_template);
  alive_children++;
  printf("DisastrOS spawning producers\n");
	disastrOS_spawn(&consFunction, (void*)&consumer_args_template);
  alive_children++;

  disastrOS_printStatus();
  int retval;
  int pid;
  while(alive_children>0 && (pid=disastrOS_wait(0, &retval))>=0){ 
    disastrOS_printStatus();
    printf("initFunction, child: %d terminated, retval:%d, alive: %d \n",
	   pid, retval, alive_children);
    --alive_children;
  }

  //Distruggo la coda di messaggi
  FixedSizeMessageQueue_destroy(&q, sem_empty, sem_full, thread_sem);

  printf("shutdown!");
  disastrOS_shutdown();
}

int main(int argc, char** argv){
  char* logfilename=0;
  if (argc>1) {
    logfilename=argv[1];
  }
  // we create the init process processes
  // the first is in the running variable
  // the others are in the ready queue
  // spawn an init process
  printf("start\n");
  disastrOS_start(initFunction, 0, logfilename);
  return 0;
}
