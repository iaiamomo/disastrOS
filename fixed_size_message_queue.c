#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>

#include "disastrOS.h"
#include "fixed_size_message_queue.h"

#define SEM_FULL 0
#define SEM_EMPTY 1
#define THREAD_SEM 2

void FixedSizeMessageQueue_init(FixedSizeMessageQueue* q,
				int size_max, int* sem_empty_fd, int* sem_full_fd, int* thread_sem_fd){
  q->messages = (char**)malloc(size_max*sizeof(char*));
  q->size=0;
  q->front_idx=0;
  q->size_max=size_max;
  q->sem_empty=SEM_EMPTY;
  q->sem_full=SEM_FULL;
  q->thread_sem=THREAD_SEM;
  *sem_empty_fd = disastrOS_semOpen(q->sem_empty,q->size_max);
  *sem_full_fd = disastrOS_semOpen(q->sem_full,0);
  *thread_sem_fd = disastrOS_semOpen(q->thread_sem,1);
}

void FixedSizeMessageQueue_pushBack(FixedSizeMessageQueue*q,
				    char* message, int sem_empty_fd, int sem_full_fd, int thread_sem_fd){
  disastrOS_semWait(sem_empty_fd);
  disastrOS_semWait(thread_sem_fd);
  int tail_idx=(q->front_idx+q->size)%q->size_max;
  q->messages[tail_idx]=message;
  ++q->size;
  disastrOS_semPost(thread_sem_fd);
  disastrOS_semPost(sem_full_fd);
}

char* FixedSizeMessageQueue_popFront(FixedSizeMessageQueue*q, int sem_empty_fd, int sem_full_fd, int thread_sem_fd){
  char* message_out=0;
  disastrOS_semWait(sem_full_fd);
  disastrOS_semWait(thread_sem_fd);
  message_out=q->messages[q->front_idx];
  q->front_idx=(q->front_idx+1)%q->size_max;
  --q->size;
  disastrOS_semPost(thread_sem_fd);
  disastrOS_semPost(sem_empty_fd);
  return message_out;
}

int FixedSizeMessageQueue_sizeMax(FixedSizeMessageQueue* q) {
  return q->size_max;
}

int FixedSizeMessageQueue_size(FixedSizeMessageQueue* q){
  return q->size; 
}

void FixedSizeMessageQueue_destroy(FixedSizeMessageQueue* q, int sem_empty_fd, int sem_full_fd, int thread_sem_fd){
  free(q->messages);
  q->size=0;
  q->front_idx=0;
  q->size_max=0;
  disastrOS_semClose(sem_empty_fd);
  disastrOS_semClose(sem_full_fd);
  disastrOS_semClose(thread_sem_fd);
}
