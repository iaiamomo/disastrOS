#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"

void internal_semClose(){
	//Prendo l'fd del semaforo da chiudere passato come argomento	
	int fd = running->syscall_args[0];
	//Cerco il descrittore del semaforo nella lista dei semafori del processo	
	SemDescriptor* s_des = SemDescriptorList_byFd(&running->sem_descriptors, fd);
	//Se il semaforo non è nella lista do errore	
	if(!s_des){
		running->syscall_retvalue = DSOS_ERESOURCECLOSE;
		return;
	}
	//1.Rimuovo il descrittore del semaforo dalla lista del processo
	s_des = (SemDescriptor*) List_detach(&running->sem_descriptors, (ListItem*)s_des);
	assert(s_des); 
	
	//Dal descrittore del semaforo mi predo il semaforo
	Semaphore* sem = s_des->semaphore;
	//2.Tolgo il puntatore al descrittore del semaforo dalla lista dei descrittori del semaforo
	SemDescriptorPtr* s_des_ptr =(SemDescriptorPtr*) List_detach(&sem->descriptors, (ListItem*)(s_des->ptr));
	assert(s_des_ptr);

	//Verifico che il semaforo non sia in uso
	if(sem->descriptors.size){
		running->syscall_retvalue = DSOS_ERESOURCEINUSE;
		return;
	}
	if(sem->waiting_descriptors.size){
		running->syscall_retvalue = DSOS_ERESOURCEINUSE;
		return;
	}
	//3.Tolgo il semaforo dalla lista dei semafori di disastrOS
	sem = (Semaphore*) List_detach(&semaphores_list, (ListItem*) sem);
	assert(sem);
	
	SemDescriptor_free(s_des);
	SemDescriptorPtr_free(s_des_ptr);
	Semaphore_free(sem);
	running->syscall_retvalue=0;
}
