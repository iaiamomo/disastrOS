#pragma once

#include "disastrOS.h"

typedef struct FixedSizeMessageQueue{
  char** messages;
  int size;
  int size_max;
  int front_idx;
  int sem_full;
  int sem_empty;
  int thread_sem;
} FixedSizeMessageQueue;

void FixedSizeMessageQueue_init(FixedSizeMessageQueue* q,
				int size_max, int* sem_empty_fd, int* sem_full_fd, int* thread_sem_fd);

void FixedSizeMessageQueue_destroy(FixedSizeMessageQueue* q, int sem_empty_fd, int sem_full_fd, int thread_sem_fd);

void FixedSizeMessageQueue_pushBack(FixedSizeMessageQueue*q,
				    char* message, int sem_empty_fd, int sem_full_fd, int thread_sem_fd);

char* FixedSizeMessageQueue_popFront(FixedSizeMessageQueue*q, int sem_empty_fd, int sem_full_fd, int thread_sem_fd);

int FixedSizeMessageQueue_sizeMax(FixedSizeMessageQueue* q);

int FixedSizeMessageQueue_size(FixedSizeMessageQueue* q);
