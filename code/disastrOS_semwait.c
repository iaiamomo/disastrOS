#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"

void internal_semWait(){
	
	//recupera argomento della syscall dal pcb del processo
	int fd = running->syscall_args[0];
	
	//cerca il semaforo nella lista dei semafori aperti dal processo chiamante
	SemDescriptor* sem_des = SemDescriptorList_byFd(&running->sem_descriptors, fd);
	
	//NB: il semaforo deve essere aperto dal processo prima di poter chiamare la wait
	// se il semaforo non è aperto dal processo la syscall termina con errore
	if(!sem_des){
		running->syscall_retvalue = DSOS_ERESOURCENOFD;
		return;
	}
	
	// decremento il contatore del semaforo
	Semaphore* sem = sem_des->semaphore;
	sem->count--;
	
	// se il contatore diventa negativo:
	// ->aggiungo il processo alla waiting_list del semaforo
	// ->metto il processo nella waiting_list del SO
	if(sem->count < 0){
		SemDescriptorPtr* sem_des_ptr = SemDescriptorPtr_alloc(sem_des);
		
		List_insert(&sem->waiting_descriptors, sem->waiting_descriptors.last, (ListItem*)sem_des_ptr);
		
		running->status = Waiting;
		running->syscall_retvalue = 0;
		List_insert(&waiting_list, waiting_list.last, (ListItem*)running);
		
		//NB: c'è sempre un processo nella ready_list (sleeper)
		PCB* next_process = (PCB*)List_detach(&ready_list, ready_list.first); 
		next_process->status = Running;										  
		running = next_process;
		
	// se contatore>=0:
	// ->la syscall termina con successo
	}else
		running->syscall_retvalue = 0;
		
}
