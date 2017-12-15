#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"

void internal_semWait(){
  // do stuff :)
	int fd = running->syscall_args[0];
	
	SemDescriptor* sem_des = SemDescriptorList_byFd(&running->sem_descriptors, fd);
	if(!sem_des){
		printf("SEMAFORO NON NEL PROCESSO\n");
		running->syscall_retvalue = DSOS_ERESOURCENOFD;
		return;
	}
	
	Semaphore* sem = sem_des->semaphore;
	sem->count--;
	
	if(sem->count < 0){
		SemDescriptorPtr* sem_des_ptr = SemDescriptorPtr_alloc(sem_des);
		
		List_insert(&sem->waiting_descriptors, sem->waiting_descriptors.last, (ListItem*)sem_des_ptr);
		
		running->status = Waiting;
		running->syscall_retvalue = 0;
		List_insert(&waiting_list, waiting_list.last, (ListItem*)running);
		
		PCB* next_process = (PCB*)List_detach(&ready_list, ready_list.first); 
		next_process->status = Running;										  
		running = next_process;
		
	}else
		running->syscall_retvalue = 0;
		
}
