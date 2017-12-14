#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"

void internal_semOpen(){
  //Recupero dal PCB l'id del semaforo
  int id=running->syscall_args[0];
  int count = running->syscall_args[1]; //count viene considerato se e solo se il semaforo deve essere ancora creato

  Semaphore* sem=SemaphoreList_byId(&semaphores_list, id);
  
  //Se il semaforo ancora non era stato allocato, lo creo
   if (!sem) {
   	if (count < 0){ //se avessi un contatore < 0 bloccherei il sistema
   		running->syscall_retvalue=DSOS_ERESOURCEOPEN;
    	return;
   	}
    sem=Semaphore_alloc(id, count);
    List_insert(&semaphores_list, semaphores_list.last, (ListItem*) sem);
  }

  // Effettuo gli eventuali controlli sul semaforo appena aperto
  if (!sem) {
     running->syscall_retvalue=DSOS_ERESOURCEOPEN;
     return;
  }
  
  
  //Creo il descrittore relativo al semaforo per il processo ed assegno il fd al semaforo
  //(Aggiunte alcune correzioni al PCB)
  SemDescriptor* des=SemDescriptor_alloc(running->last_sem_fd, sem, running);
  if (! des){
     running->syscall_retvalue=DSOS_ERESOURCENOFD;
     return;
  }
  running->last_sem_fd++;
  SemDescriptorPtr* desptr=SemDescriptorPtr_alloc(des);
  List_insert(&running->sem_descriptors, running->sem_descriptors.last, (ListItem*) des);
  
  //Aggiungo alla lista dei descrittori, il descrittore del semaforo appena aperto
  des->ptr=desptr;
  List_insert(&sem->descriptors, sem->descriptors.last, (ListItem*) desptr);

  // Ritorno il fd del descrittore appena creato
  running->syscall_retvalue = des->fd;
}
