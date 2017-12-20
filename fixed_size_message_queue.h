#pragma once

#include "disastrOS.h"

#define MESSAGE_SIZE 32
#define MESSAGE_MEM_SIZE (MESSAGE_SIZE + sizeof(int))

#define MAX_NUM_MESSAGES 16

typedef struct FixedSizeMessageQueue{
  char* messages[MAX_NUM_MESSAGES];
  int size;
  int size_max;
  int front_idx;
  int sem_full;
  int sem_empty;
  int thread_sem;
} FixedSizeMessageQueue;

char* Message_alloc(char*buf);
int Message_free(char* msg);



void FixedSizeMessageQueue_init();

FixedSizeMessageQueue* FixedSizeMessageQueue_alloc(int* sem_empty_fd, int* sem_full_fd, int* thread_sem_fd);

void FixedSizeMessageQueue_destroy(FixedSizeMessageQueue* q, int sem_empty_fd, int sem_full_fd, int thread_sem_fd);

void FixedSizeMessageQueue_pushBack(FixedSizeMessageQueue*q,
				    char* buf, int sem_empty_fd, int sem_full_fd, int thread_sem_fd);

char* FixedSizeMessageQueue_popFront(FixedSizeMessageQueue*q, int sem_empty_fd, int sem_full_fd, int thread_sem_fd);

int FixedSizeMessageQueue_sizeMax(FixedSizeMessageQueue* q);

int FixedSizeMessageQueue_size(FixedSizeMessageQueue* q);
