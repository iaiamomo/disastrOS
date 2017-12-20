#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pool_allocator.h"
#include "disastrOS.h"
#include "fixed_size_message_queue.h"
#include "disastrOS_constants.h"

#define SEM_FULL 0
#define SEM_EMPTY 1
#define THREAD_SEM 2

#define QUEUE_SIZE (sizeof(FixedSizeMessageQueue))
#define QUEUE_MEM_SIZE (QUEUE_SIZE+sizeof(int))
#define MAX_NUM_QUEUES 1
#define QUEUE_BUFFER_SIZE (MAX_NUM_QUEUES * QUEUE_MEM_SIZE)



#define MESSAGE_BUFFER_SIZE ( (MAX_NUM_MESSAGES)*(MESSAGE_MEM_SIZE) )

static char _queue_buffer[QUEUE_BUFFER_SIZE]; 	// area di memoria gestita da SO in cui il SO allocherà le code
static PoolAllocator _queue_allocator;	      	// allocatore delle code

//static char _queue_ptr_buffer[QUEUEPTR_BUFFER_SIZE]; // area di memoria in cui il SO allocherà i puntatori alle code
//static PoolAllocator _queue_ptr_allocator;           // allocatore dei puntatori alle code


static char _message_buffer[MESSAGE_BUFFER_SIZE];	// area di memoria in cui il SO allocherà i messaggi
static PoolAllocator _message_allocator;

//static char _message_ptr_buffer[MESSAGEPTR_BUFFER_SIZE];// area di memoria in cui il SO allocherà i puntatori dei messaggi
//static PoolAllocator _message_ptr_allocator;


void FixedSizeMessageQueue_init(){
  int result=PoolAllocator_init(& _queue_allocator,
				QUEUE_SIZE,
				MAX_NUM_QUEUES,
				_queue_buffer,
				QUEUE_BUFFER_SIZE);
  printf("res:%d\n",result);
  assert(! result);
/*
  result=PoolAllocator_init(& _queue_ptr_allocator,
			    QUEUEPTR_SIZE,
			    MAX_NUM_PROCESSES,
			    _queue_ptr_buffer,
			    QUEUEPTR_BUFFER_SIZE);
  assert(! result);
*/
  result=PoolAllocator_init(& _message_allocator,
			    MESSAGE_SIZE,
			    MAX_NUM_MESSAGES,
			    _message_buffer,
			    MESSAGE_BUFFER_SIZE);
  assert(! result);
/*
  result=PoolAllocator_init(& _message_ptr_allocator,
			    MESSAGEPTR_SIZE,
			    MAX_NUM_PROCESSES,
			    _message_ptr_buffer,
			    MESSAGEPTR_BUFFER_SIZE);
  assert(! result);
*/
}

char* Message_alloc(char*buf){
  assert(strlen(buf)<=MESSAGE_SIZE);
  char* msg= (char*)PoolAllocator_getBlock(&_message_allocator);
  strcpy(msg,buf);
  return msg;
}

int Message_free(char* msg){
  return PoolAllocator_releaseBlock(&_message_allocator, msg);
}

FixedSizeMessageQueue* FixedSizeMessageQueue_alloc(int* sem_empty_fd, int* sem_full_fd, int* thread_sem_fd){
  FixedSizeMessageQueue* q=(FixedSizeMessageQueue*)PoolAllocator_getBlock(&_queue_allocator);
  //q->messages = (char**)malloc(size_max*sizeof(char*));
  q->size=0;
  q->front_idx=0;
  q->size_max=MAX_NUM_MESSAGES;
  q->sem_empty=SEM_EMPTY;
  q->sem_full=SEM_FULL;
  q->thread_sem=THREAD_SEM;
  *sem_empty_fd = disastrOS_semOpen(q->sem_empty,q->size_max);
  *sem_full_fd = disastrOS_semOpen(q->sem_full,0);
  *thread_sem_fd = disastrOS_semOpen(q->thread_sem,1);

  return q;
}

void FixedSizeMessageQueue_pushBack(FixedSizeMessageQueue*q,
				    char* buf, int sem_empty_fd, int sem_full_fd, int thread_sem_fd){
  disastrOS_semWait(sem_empty_fd);
  disastrOS_semWait(thread_sem_fd);
  int tail_idx=(q->front_idx+q->size)%q->size_max;
  char* message=(char*)Message_alloc(buf); //Il messaggio deve essere allocato in sezione critica,
  q->messages[tail_idx]=message;			//così sono certo che ho blocchi liberi nel buffer
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
  return message_out;			//NB: il messaggio rimane nel _message_buffer, sarà cura del programmatore
}					//    deallocare il messaggio per non causare memory leack

int FixedSizeMessageQueue_sizeMax(FixedSizeMessageQueue* q) {
  return q->size_max;
}

int FixedSizeMessageQueue_size(FixedSizeMessageQueue* q){
  return q->size;
}

void FixedSizeMessageQueue_destroy(FixedSizeMessageQueue* q, int sem_empty_fd, int sem_full_fd, int thread_sem_fd){
  //free(q->messages);
  //q->size=0;
  //q->front_idx=0;
  //q->size_max=0;
  int i;
  for(i=0;i<q->size;i++)                                            // quando dealloco la coda e messaggi al suo interno
    PoolAllocator_releaseBlock(&_message_allocator,q->messages[i]); // vengono distrutti

  PoolAllocator_releaseBlock(&_queue_allocator, q);

  disastrOS_semClose(sem_empty_fd);
  disastrOS_semClose(sem_full_fd);
  disastrOS_semClose(thread_sem_fd);
}
