#include <stdio.h>
#include <unistd.h>
#include <poll.h>

#include "disastrOS.h"

// we need this to handle the sleep state
void sleeperFunction(void* args){
  printf("Hello, I am the sleeper, and I sleep %d\n",disastrOS_getpid());
  while(1) {
    getc(stdin);
    disastrOS_printStatus();
  }
}

void childFunction(void* args){
  disastrOS_printStatus();
  printf("child:::%d started\n",disastrOS_getpid());

  int sem_id = 1;
  int sem_count = 1;
  int sem_fd = disastrOS_semOpen(sem_id,sem_count);
  if(sem_fd<0){
    printf("child:::%d could not open sem:%d\n",disastrOS_getpid(),sem_id);
    disastrOS_exit(-1);  
  }

  printf("child:::%d entering critical section\n",disastrOS_getpid());

  int res = disastrOS_semWait(sem_fd);
  if(res<0){
    printf("child:::%d could not wait on sem:%d\n",disastrOS_getpid(),sem_id);
    disastrOS_exit(-1);  
  }
  printf("child:::%d in       critical section\n",disastrOS_getpid());
  disastrOS_sleep(5);
  printf("child:::%d exiting  critical section\n",disastrOS_getpid());

  res = disastrOS_semPost(sem_fd);
  if(res<0){
    printf("child:::%d could not post on sem:%d\n",disastrOS_getpid(),sem_id);
    disastrOS_exit(-1);  
  }

  printf("child:::%d out of   critical section\n",disastrOS_getpid());	

  printf("child:::%d terminated\n",disastrOS_getpid());
  disastrOS_exit(disastrOS_getpid()+1);
}


void initFunction(void* args) {
  disastrOS_printStatus();
  printf("hello, I am init and I just started\n");
  printf("DisastrOS spawning sleeper thread\n");
  disastrOS_spawn(sleeperFunction, 0);
  

  printf("DisastrOS spawning 10 threads ready to access CS\n");
  int alive_children=0;
  for (int i=0; i<10; ++i) {
    disastrOS_spawn(childFunction, 0);
    alive_children++;
  }

  disastrOS_printStatus();
  int retval;
  int pid;
  while(alive_children>0 && (pid=disastrOS_wait(0, &retval))>=0){ 
    disastrOS_printStatus();
    printf("initFunction, child: %d terminated, retval:%d, alive: %d \n",
	   pid, retval, alive_children);
    --alive_children;
  }
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
  printf("the function pointer is: %p", childFunction);
  // spawn an init process
  printf("start\n");
  disastrOS_start(initFunction, 0, logfilename);
  return 0;
}
